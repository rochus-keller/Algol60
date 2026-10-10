/*
* Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the AIR (Algol Intermediate Representation) library.
*
* The following is the license that applies to this copy of the
* file. For a license to use the file under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include "AirLjGen.h"
#include <LjTools/LuaJitComposer2.h>
#include <QIODevice>
#include <QtDebug>
using namespace Air;

/*
    The generated module chunk uses the following slots:

       slot 0: the AirRt runtime table, required at load time
       slot 1: the module table, which is also the return value of the chunk
       slot 2..n: one table per imported module, required at load time

    The procedures of the module are nested functions of the module chunk which
    access these slots as upvalues
    Module level declarations are fields of the module table, indexed by name
*/

class LjGen::Imp
{
public:
    struct NoMoreFreeSlots {};

    struct Ctx
    {
        Declaration* scope;
        Lua::JitComposer::SlotPool pool;
        typedef QHash<quint8,QPair<quint16,QByteArray> > Upvals;
        Upvals upvals;

        Ctx(Declaration* s = 0):scope(s) {}

        int buySlots(int len = 1, bool call = false)
        {
            const int tmp = Lua::JitComposer::nextFreeSlot(pool,len,call);
            if( tmp < 0 )
                throw NoMoreFreeSlots();
            return tmp;
        }
        void sellSlots(quint8 base, int len = 1)
        {
            Lua::JitComposer::releaseSlot(pool, base, len);
        }
        quint16 resolveUpval(quint8 slot, const QByteArray& name)
        {
            Upvals::const_iterator i = upvals.find(slot);
            if( i != upvals.end() )
                return i.value().first;
            const int nr = upvals.size();
            QPair<quint16,QByteArray>& uv = upvals[slot];
            uv.first = nr;
            uv.second = name;
            return nr;
        }
    };

    struct Val
    {
        quint8 slot;
        Type* t;
        quint8 st;
        Val(quint8 s = 0, Type* t = 0, quint8 st = ST_void):slot(s),t(t),st(st) {}
    };

    AstModel* mdl;
    Lua::JitComposer bc;
    Declaration* module;
    Declaration* curProc;
    QList<Ctx> ctx;
    QList<Val> stack; // evaluation stack, mapped to slots
    QList<quint8> localSlots; // local id -> slot
    QList<quint8> argSlots; // argument id -> slot
    QSet<quint32> boxedLocals; // local ids which are the target of a refloc
    QSet<quint32> boxedArgs; // argument ids which are the target of a refarg
    QList<QList<quint32> > loopExits; // one list of pending jumps per open LOOP
    QHash<QByteArray,quint32> labelPc; // label -> pc of its first instruction - 1
    QHash<QByteArray,QList<quint32> > pendingGotos;
    quint8 rtSlot, modSlot;
    QHash<QByteArray,quint8> importSlots; // module name -> slot in the module chunk
    RowCol curPos;
    QList<LjGen::Error> errors;

    Imp():mdl(0),module(0),curProc(0),rtSlot(0),modSlot(0) {}

    static QByteArray sanitize(const QByteArray& name)
    {
        QByteArray res = name;
        for( int i = 0; i < res.size(); i++ )
        {
            const char ch = res[i];
            if( !( ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) ||
                   ( ch >= '0' && ch <= '9' ) ) )
                res[i] = '_';
        }
        if( !res.isEmpty() && res[0] >= '0' && res[0] <= '9' )
            res = "_" + res;
        return res;
    }

    static QByteArray keyOf(Declaration* d)
    {
        if( d == 0 )
            return QByteArray();
        QByteArray path = d->name;
        if( d->typebound && d->outer && d->outer->kind == Declaration::TypeDecl )
            path = d->outer->name + "." + path;
        return path;
    }

    static quint32 packed(const RowCol& p)
    {
        return Lua::JitRowCol::packRowCol(p.d_row, p.d_col ? p.d_col : 1);
    }

    quint32 pos(const RowCol& p)
    {
        if( p.isValid() )
            return packed(p);
        if( curPos.isValid() )
            return packed(curPos);
        return packed(RowCol(1,1));
    }

    static quint8 hashBits(int count)
    {
        quint8 bits = 0;
        while( count > ( 1 << bits ) && bits < 31 )
            bits++;
        return bits;
    }

    static quint16 arrSize(int count)
    {
        return count < 0 ? 0 : ( count > 2047 ? 2047 : quint16(count) );
    }

    static void maxRow(Expression* e, quint32& m)
    {
        while( e )
        {
            if( e->pos.d_row > m )
                m = e->pos.d_row;
            maxRow(e->e, m);
            e = e->next;
        }
    }

    static void maxRow(Statement* s, quint32& m)
    {
        while( s )
        {
            if( s->pos.d_row > m )
                m = s->pos.d_row;
            maxRow(s->e, m);
            maxRow(s->body, m);
            s = s->next;
        }
    }

    static void maxRow(Declaration* d, quint32& m)
    {
        while( d )
        {
            if( d->pos.d_row > m )
                m = d->pos.d_row;
            if( d->kind == Declaration::Procedure )
            {
                ProcedureData* pd = d->getPd();
                if( pd && pd->end.d_row > m )
                    m = pd->end.d_row;
                maxRow(d->body, m);
            }else if( d->kind == Declaration::Module )
            {
                ModuleData* md = d->getMd();
                if( md && md->end.d_row > m )
                    m = md->end.d_row;
            }
            maxRow(d->subs, m);
            if( d->kind == Declaration::TypeDecl && d->getType() )
            {
                // the bound procedures of a record are owned by its type
                Type* t = d->getType();
                for( int i = 0; i < t->subs.size(); i++ )
                    if( t->subs[i]->kind == Declaration::Procedure )
                    {
                        quint32 r = lastRow(t->subs[i]).d_row;
                        if( r > m )
                            m = r;
                    }
            }
            d = d->next;
        }
    }

    static RowCol lastRow(Declaration* d)
    {
        // the composer requires each instruction line to be within the line range
        // of its function, and the range determines the width of the line table
        quint32 m = d->pos.d_row;
        if( d->kind == Declaration::Procedure )
        {
            ProcedureData* pd = d->getPd();
            if( pd && pd->end.d_row > m )
                m = pd->end.d_row;
            maxRow(d->body, m);
            // the initializers of the locals use their declaration position
            maxRow(d->subs, m);
        }else if( d->kind == Declaration::Module )
        {
            ModuleData* md = d->getMd();
            if( md && md->end.d_row > m )
                m = md->end.d_row;
            maxRow(d->subs, m);
        }
        // the last column, so that the range covers each instruction of the last row
        return RowCol(m, ( 1 << RowCol::COL_BIT_LEN ) - 1);
    }

    static RowCol firstRow(Declaration* d)
    {
        return RowCol(d->pos.d_row, 1);
    }

    void error(const QString& msg, const RowCol& p)
    {
        LjGen::Error e;
        e.msg = msg;
        e.pc = p.isValid() ? p.d_row : curPos.d_row;
        if( module )
            e.where = module->name;
        errors << e;
    }

    Type* typeOf(Declaration* d) const
    {
        if( d == 0 )
            return 0;
        Type* t = d->getType();
        return t ? t->deref() : 0;
    }

    static bool isBool(const Val& v)
    {
        return v.t && v.t->deref()->kind == Type::BOOL;
    }

    static bool isRef(quint8 st)
    {
        return st == ST_ref || st == ST_nil || st == ST_procref ||
               st == ST_methref || st == ST_task || st == ST_desig;
    }


    void push(quint8 slot, Type* t)
    {
        stack.push_back(Val(slot, t, t ? t->stackType() : ST_void));
    }
    void push(quint8 slot, Type* t, quint8 st)
    {
        stack.push_back(Val(slot, t, st));
    }
    Val top(int down = 0) const
    {
        const int i = stack.size() - 1 - down;
        if( i < 0 )
            return Val();
        return stack[i];
    }
    void releaseSlot()
    {
        if( stack.isEmpty() )
            return;
        ctx.back().sellSlots(stack.back().slot);
        stack.pop_back();
    }
    bool require(int n, const RowCol& p)
    {
        if( stack.size() < n )
        {
            error("the evaluation stack is empty", p);
            return false;
        }
        return true;
    }


    void fetchModuleTable(quint8 to, const RowCol& p)
    {
        if( ctx.back().scope == module )
            bc.MOV(to, modSlot, pos(p));
        else
            bc.UGET(to, ctx.back().resolveUpval(modSlot,"@mod"), pos(p));
    }

    void fetchRuntime(quint8 to, const RowCol& p)
    {
        if( ctx.back().scope == module )
            bc.MOV(to, rtSlot, pos(p));
        else
            bc.UGET(to, ctx.back().resolveUpval(rtSlot,"@rt"), pos(p));
    }

    void fetchImport(quint8 to, const QByteArray& name, const RowCol& p)
    {
        QHash<QByteArray,quint8>::const_iterator i = importSlots.find(name);
        if( i == importSlots.end() )
        {
            error(QString("module '%1' is used but not imported").arg(name.constData()), p);
            bc.KNIL(to, 1, pos(p));
            return;
        }
        if( ctx.back().scope == module )
            bc.MOV(to, i.value(), pos(p));
        else
            bc.UGET(to, ctx.back().resolveUpval(i.value(),"@" + name), pos(p));
    }

    // to := the table which owns d, i.e. the module table or an import table
    void fetchOwner(quint8 to, Declaration* d, const RowCol& p)
    {
        Declaration* m = d ? d->getModule() : 0;
        if( m == 0 || m == module )
            fetchModuleTable(to, p);
        else
            fetchImport(to, m->name, p);
    }

    // to := <owner>[<key>], the value of a module level declaration
    void fetchDecl(quint8 to, Declaration* d, const RowCol& p)
    {
        if( d == 0 )
        {
            bc.KNIL(to, 1, pos(p));
            return;
        }
        fetchOwner(to, d, p);
        getField(to, to, keyOf(d), p);
    }

    bool constFits(const QByteArray& name)
    {
        return bc.getConstSlot(QVariant::fromValue(name)) <= 255;
    }

    void getField(quint8 to, quint8 table, const QByteArray& name, const RowCol& p)
    {
        if( constFits(name) )
        {
            bc.TGET(to, table, name, pos(p));
            return;
        }
        const quint8 k = ctx.back().buySlots(1);
        bc.KSET(k, name, pos(p));
        bc.TGET(to, table, k, pos(p));
        ctx.back().sellSlots(k);
    }

    void setField(quint8 value, quint8 table, const QByteArray& name, const RowCol& p)
    {
        if( constFits(name) )
        {
            bc.TSET(value, table, name, pos(p));
            return;
        }
        const quint8 k = ctx.back().buySlots(1);
        bc.KSET(k, name, pos(p));
        bc.TSET(value, table, k, pos(p));
        ctx.back().sellSlots(k);
    }

    void fetchTypeObject(quint8 to, Type* t, const RowCol& p)
    {
        if( t == 0 || t->decl == 0 )
            bc.KNIL(to, 1, pos(p));
        else
            fetchDecl(to, t->decl, p);
    }

    static QByteArray cType(Type* t)
    {
        if( t == 0 )
            return "void";
        t = t->deref();
        if( t == 0 )
            return QByteArray();
        switch( t->kind ) {
        case Type::BOOL:
            return "int"; // a boolean is zero or one in this backend
        case Type::CHAR:
        case Type::INT8:
            return "int8_t";
        case Type::INT16:
            return "int16_t";
        case Type::INT32:
            return "int32_t";
        case Type::INT64:
            return "int64_t";
        case Type::UINT8:
            return "uint8_t";
        case Type::UINT16:
            return "uint16_t";
        case Type::UINT32:
            return "uint32_t";
        case Type::UINT64:
            return "uint64_t";
        case Type::FLOAT32:
            return "float";
        case Type::FLOAT64:
            return "double";
        case Type::Pointer:
            {
                // POINTER TO ARRAY OF CHAR is a string, which the FFI converts
                Type* to = t->getType() ? t->getType()->deref() : 0;
                if( to && to->kind == Type::Array && to->getType() &&
                        to->getType()->deref() && to->getType()->deref()->kind == Type::CHAR )
                    return "const char*";
                return QByteArray();
            }
        case Type::CPointer:
            return "void*";
        default:
            return QByteArray();
        }
    }

    static char cKind(Type* t, bool varParam)
    {
        // how the wrapper converts an argument or the result
        if( t == 0 )
            return 'v';
        Type* d = t->deref();
        if( d == 0 )
            return 0;
        if( d->kind == Type::Pointer )
            return varParam ? 0 : 's'; // a string cannot be written back
        if( d->isNumber() || d->kind == Type::CHAR || d->kind == Type::BOOL )
            return varParam ? 'P' : 'n';
        return 0;
    }

    void emitForeignBinding(Declaration* proc)
    {
        const QByteArray binding = proc->pd && !proc->pd->externalName.isEmpty() ?
                    proc->pd->externalName : proc->name;
        // the external name is the symbol, optionally prefixed by the library it is in
        const int colon = binding.indexOf(':');
        const QByteArray lib = colon < 0 ? QByteArray() : binding.left(colon);
        const QByteArray sym = colon < 0 ? binding : binding.mid(colon+1);

        const QByteArray res = cType(proc->getType());
        QByteArray cdecl_ = res + " " + sym + "(";
        QByteArray desc;
        bool ok = !res.isEmpty();

        const char rk = proc->getType() == 0 ? 'v' : cKind(proc->getType(), false);
        if( rk == 0 )
            ok = false;
        desc += rk;

        const DeclList params = proc->getParams();
        for( int i = 0; i < params.size(); i++ )
        {
            Type* t = params[i]->getType();
            QByteArray ct = cType(t);
            const char k = cKind(t, params[i]->varParam);
            if( ct.isEmpty() || ct == "void" || k == 0 )
            {
                ok = false;
                break;
            }
            if( i > 0 )
                cdecl_ += ", ";
            if( params[i]->varParam )
            {
                cdecl_ += ct + "*";
                desc += QByteArray(";") + k + ":" + ct;
            }else
            {
                cdecl_ += ct;
                desc += QByteArray(";") + k;
            }
            cdecl_ += " " + params[i]->name;
        }
        cdecl_ += ");";

        if( !ok )
        {
            error(QString("the signature of the FOREIGN procedure '%1' cannot be mapped to C").
                  arg(proc->name.constData()), proc->pos);
            return;
        }

        const int cfs = bc.getCallFrameSize();
        const int base = beginRtCall("cproc", 4, proc->pos);
        bc.KSET(base + cfs, cdecl_, pos(proc->pos));
        bc.KSET(base + cfs + 1, lib, pos(proc->pos));
        bc.KSET(base + cfs + 2, sym, pos(proc->pos));
        bc.KSET(base + cfs + 3, desc, pos(proc->pos));
        bc.CALL(base, 1, 4, pos(proc->pos));
        setField(base, modSlot, keyOf(proc), proc->pos);
        ctx.back().sellSlots(base, cfs + 4);
    }

    // buys the call frame and puts A.<fn> into its base
    int beginRtCall(const QByteArray& fn, int nargs, const RowCol& p)
    {
        const int cfs = bc.getCallFrameSize();
        const int base = ctx.back().buySlots(cfs + nargs, true);
        fetchRuntime(base, p);
        getField(base, base, fn, p);
        return base;
    }

    // moves the result to the lowest of the nOperands slots on the evaluation stack
    // releases the call frame and all operands but the result
    void endCall(int base, int frameLen, int nOperands, bool wantResult,
                 Type* resType, quint8 resSt, const RowCol& p)
    {
        if( wantResult )
        {
            if( nOperands > 0 )
            {
                const quint8 res = stack[stack.size()-nOperands].slot;
                bc.MOV(res, base, pos(p));
                ctx.back().sellSlots(base, frameLen);
                for( int i = 1; i < nOperands; i++ )
                    releaseSlot();
                stack.back() = Val(res, resType, resType ? resType->stackType() : resSt);
            }else
            {
                ctx.back().sellSlots(base, frameLen);
                const quint8 res = ctx.back().buySlots(1);
                if( res != base )
                    bc.MOV(res, base, pos(p));
                push(res, resType, resType ? resType->stackType() : resSt);
            }
        }else
        {
            ctx.back().sellSlots(base, frameLen);
            for( int i = 0; i < nOperands; i++ )
                releaseSlot();
        }
    }


    void emitJump(qint16 offset, const RowCol& p)
    {
        const quint8 base = Lua::JitComposer::currentHighWater(ctx.back().pool);
        bc.JMP(base, offset, pos(p));
    }

    quint32 emitPendingJump(const RowCol& p)
    {
        emitJump(0, p);
        return bc.getCurPc();
    }

    quint32 emitJumpIfFalse(const Val& v, const RowCol& p)
    {
        if( isRef(v.st) )
            bc.ISF(v.slot, pos(p));
        else
            bc.ISEQ(v.slot, QVariant(0), pos(p));
        return emitPendingJump(p);
    }

    quint32 emitJumpIfTrue(const Val& v, const RowCol& p)
    {
        if( isRef(v.st) )
            bc.IST(v.slot, pos(p));
        else
            bc.ISNE(v.slot, QVariant(0), pos(p));
        return emitPendingJump(p);
    }


    static QVariant defaultOf(Type* t)
    {
        if( t == 0 )
            return QVariant();
        t = t->deref();
        if( t->isNumber() || t->kind == Type::BOOL || t->kind == Type::CHAR )
            return QVariant(0);
        return QVariant();
    }

    void emitInitializer(quint8 slot, Type* t, const RowCol& p)
    {
        const QVariant v = defaultOf(t);
        if( v.isValid() )
            bc.KSET(slot, v, pos(p));
        else
            bc.KNIL(slot, 1, pos(p));
    }


    bool translate(Declaration* m, QIODevice* out)
    {
        module = m;
        errors.clear();
        importSlots.clear();
        curPos = m->pos;

        ModuleData* md = m->getMd();
        const QByteArray source = md && !md->source.isEmpty() ?
                    md->source.toUtf8() : m->name;
        const RowCol end = lastRow(m);

        ctx.push_back(Ctx(m));
        bc.openFunction(0, source, pos(firstRow(m)), pos(end));

        QHash<quint8,QByteArray> names;
        rtSlot = ctx.back().buySlots(1);
        names[rtSlot] = "@rt";
        modSlot = ctx.back().buySlots(1);
        names[modSlot] = "@mod";

        emitRequire(rtSlot, "AirRt", m->pos);
        bc.TNEW(modSlot, 0, 0, pos(m->pos));

        // the runtime finds the name of a procedure in the module table
        {
            const int cfs = bc.getCallFrameSize();
            const int base = beginRtCall("regmod", 2, m->pos);
            bc.MOV(base + cfs, modSlot, pos(m->pos));
            bc.KSET(base + cfs + 1, sanitize(m->name), pos(m->pos));
            bc.CALL(base, 0, 2, pos(m->pos));
            ctx.back().sellSlots(base, cfs + 2);
        }

        Declaration* sub = m->subs;
        while( sub )
        {
            if( sub->kind == Declaration::Import )
            {
                const quint8 slot = ctx.back().buySlots(1);
                names[slot] = "@" + sub->name;
                importSlots.insert(sub->name, slot);
                emitRequire(slot, sanitize(sub->name), sub->pos);
            }
            sub = sub->next;
        }

        for( sub = m->subs; sub; sub = sub->next )
            if( sub->kind == Declaration::Procedure && sub->extern_ )
            {
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("extern", 2, sub->pos);
                bc.KSET(base + cfs, m->name, pos(sub->pos));
                bc.KSET(base + cfs + 1, sub->pd && !sub->pd->externalName.isEmpty() ?
                            sub->pd->externalName : sub->name, pos(sub->pos));
                bc.CALL(base, 1, 2, pos(sub->pos));
                setField(base, modSlot, keyOf(sub), sub->pos);
                ctx.back().sellSlots(base, cfs + 2);
            }

        for( sub = m->subs; sub; sub = sub->next )
        {
            if( sub->kind == Declaration::Procedure && sub->foreign_ )
                emitForeignBinding(sub);
        }

        for( sub = m->subs; sub; sub = sub->next )
        {
            if( sub->kind == Declaration::TypeDecl )
                emitTypeDecl(sub);
        }

        for( sub = m->subs; sub; sub = sub->next )
        {
            if( sub->kind == Declaration::VarDecl )
            {
                const quint8 val = ctx.back().buySlots(1);
                emitInitializer(val, sub->getType(), sub->pos);
                setField(val, modSlot, keyOf(sub), sub->pos);
                ctx.back().sellSlots(val);
            }
        }

        for( sub = m->subs; sub; sub = sub->next )
        {
            Declaration* proc = sub;
            if( sub->kind == Declaration::Placeholder )
                proc = sub->imported; // a bound procedure, declared in its receiver type
            if( proc && proc->kind == Declaration::Procedure &&
                    !proc->extern_ && !proc->foreign_ )
                emitProcedure(proc);
        }

        for( sub = m->subs; sub; sub = sub->next )
            if( sub->kind == Declaration::Procedure && sub->init )
            {
                const int cfs = bc.getCallFrameSize();
                const int base = ctx.back().buySlots(cfs, true);
                getField(base, modSlot, keyOf(sub), end);
                bc.CALL(base, 0, 0, pos(end));
                ctx.back().sellSlots(base, cfs);
            }

        bc.UCLO(0, 0, pos(end));
        bc.RET(modSlot, 1, pos(end));

        setNames(names);
        bc.closeFunction(ctx.back().pool.d_frameSize);
        ctx.pop_back();

        if( errors.isEmpty() )
            bc.write(out);
        return errors.isEmpty();
    }

    void setNames(const QHash<quint8,QByteArray>& names)
    {
        Lua::JitComposer::VarNameList sn(ctx.back().pool.d_frameSize);
        QHash<quint8,QByteArray>::const_iterator vi;
        for( vi = names.begin(); vi != names.end(); ++vi )
        {
            if( vi.key() >= sn.size() )
                continue;
            Lua::JitComposer::VarName& n = sn[vi.key()];
            n.d_name = vi.value();
            n.d_to = bc.getCurPc();
        }
        bc.setVarNames(sn);

        Lua::JitComposer::UpvalList uv(ctx.back().upvals.size());
        Ctx::Upvals::const_iterator uvi;
        for( uvi = ctx.back().upvals.begin(); uvi != ctx.back().upvals.end(); ++uvi )
        {
            Lua::JitComposer::Upval u;
            u.d_name = uvi.value().second;
            u.d_isRo = true;
            u.d_isLocal = true;
            u.d_uv = uvi.key();
            uv[uvi.value().first] = u;
        }
        bc.setUpvals(uv);
    }

    void emitRequire(quint8 to, const QByteArray& name, const RowCol& p)
    {
        const int cfs = bc.getCallFrameSize();
        const int base = ctx.back().buySlots(cfs + 1, true);
        bc.GGET(base, "require", pos(p));
        bc.KSET(base + cfs, name, pos(p));
        bc.CALL(base, 1, 1, pos(p));
        bc.MOV(to, base, pos(p));
        ctx.back().sellSlots(base, cfs + 1);
    }

    void emitTypeDecl(Declaration* d)
    {
        Type* t = d->getType();
        if( t == 0 )
            return;
        t = t->deref();
        if( t->kind != Type::Record && t->kind != Type::Struct &&
                t->kind != Type::CStruct && t->kind != Type::CUnion )
            return;

        if( t->kind == Type::Record )
        {
            // M.<name> = A.rectype('<module>!<name>', <base>)
            const int cfs = bc.getCallFrameSize();
            const int base = ctx.back().buySlots(cfs + 2, true);
            fetchRuntime(base, d->pos);
            getField(base, base, "rectype", d->pos);
            bc.KSET(base + cfs, module->name + "!" + d->name, pos(d->pos));
            Type* super = t->getType() ? t->getType()->deref() : 0;
            fetchTypeObject(base + cfs + 1, super, d->pos);
            bc.CALL(base, 1, 2, pos(d->pos));
            setField(base, modSlot, keyOf(d), d->pos);
            ctx.back().sellSlots(base, cfs + 2);
        }

        // M.<name>$p is the prototype of an instance
        const quint8 proto = ctx.back().buySlots(1);
        const DeclList fields = t->getFieldList(true);
        bc.TNEW(proto, 0, hashBits(fields.size()), pos(d->pos));
        if( !fields.isEmpty() )
        {
            const quint8 val = ctx.back().buySlots(1);
            for( int i = 0; i < fields.size(); i++ )
            {
                emitInitializer(val, fields[i]->getType(), d->pos);
                setField(val, proto, fields[i]->name, d->pos);
            }
            ctx.back().sellSlots(val);
        }
        setField(proto, modSlot, keyOf(d) + "$p", d->pos);
        ctx.back().sellSlots(proto);
    }


    void collectRefs(Statement* s)
    {
        for( ; s; s = s->next )
        {
            collectRefs(s->e);
            if( s->body )
                collectRefs(s->body);
        }
    }

    void collectRefs(Expression* e)
    {
        for( ; e; e = e->next )
        {
            if( e->kind == op_refloc )
                boxedLocals.insert(e->id);
            else if( e->kind == op_refarg )
                boxedArgs.insert(e->id);
            if( e->e )
                collectRefs(e->e);
        }
    }

    void emitProcedure(Declaration* p)
    {
        const DeclList params = p->getParams();
        const DeclList locals = p->getLocals();
        const RowCol end = lastRow(p);

        curProc = p;
        curPos = p->pos;
        stack.clear();
        localSlots.clear();
        argSlots.clear();
        boxedLocals.clear();
        boxedArgs.clear();
        loopExits.clear();
        labelPc.clear();
        pendingGotos.clear();
        collectRefs(p->body);

        ctx.push_back(Ctx(p));
        const int id = bc.openFunction(params.size(),
                                       module->name + "!" + keyOf(p),
                                       pos(firstRow(p)), pos(end));

        QHash<quint8,QByteArray> names;
        for( int i = 0; i < params.size(); i++ )
        {
            const quint8 slot = ctx.back().buySlots(1);
            argSlots << slot;
            names[slot] = sanitize(params[i]->name);
        }
        for( int i = 0; i < locals.size(); i++ )
        {
            const quint8 slot = ctx.back().buySlots(1);
            localSlots << slot;
            names[slot] = sanitize(locals[i]->name);
        }
        for( int i = 0; i < params.size(); i++ )
        {
            // a VAR parameter is a designator already and is forwarded as it is
            if( isBoxed(quint32(i), true) )
                emitBox(argSlots[i], true, params[i]->pos);
        }
        for( int i = 0; i < locals.size(); i++ )
        {
            emitInitializer(localSlots[i], locals[i]->getType(), locals[i]->pos);
            if( boxedLocals.contains(quint32(i)) )
                emitBox(localSlots[i], true, locals[i]->pos);
        }

        statementSeq(p->body);

        // a Lua function must end with a return
        bc.RET(pos(end));

        QHash<QByteArray,QList<quint32> >::const_iterator i;
        for( i = pendingGotos.begin(); i != pendingGotos.end(); ++i )
        {
            if( !i.value().isEmpty() )
                error(QString("goto to the undefined label '%1'").arg(i.key().constData()),
                      p->pos);
        }

        setNames(names);
        bc.closeFunction(ctx.back().pool.d_frameSize);
        ctx.pop_back();
        curProc = 0;

        // install the procedure in the module table
        const int tmp = ctx.back().buySlots(1);
        bc.FNEW(tmp, id, pos(end));
        setField(tmp, modSlot, keyOf(p), end);
        if( p->typebound && p->outer && p->outer->kind == Declaration::TypeDecl )
        {
            // a bound procedure is also a field of the record type object
            // so that it can be selected by the dynamic type of a receiver
            const int t = ctx.back().buySlots(1);
            fetchDecl(t, p->outer, end);
            setField(tmp, t, p->name, end);
            ctx.back().sellSlots(t);
        }
        ctx.back().sellSlots(tmp);
    }

    void emitBox(quint8 slot, bool hasValue, const RowCol& p)
    {
        const quint8 tmp = ctx.back().buySlots(1);
        bc.TNEW(tmp, 1, 0, pos(p));
        if( hasValue )
            bc.TSETi(slot, tmp, 1, pos(p));
        bc.MOV(slot, tmp, pos(p));
        ctx.back().sellSlots(tmp);
    }


    void statementSeq(Statement* s)
    {
        while( s )
        {
            emitStatement(s);
            s = s->next;
        }
    }

    void emitStatement(Statement*& s)
    {
        if( s->pos.isValid() )
            curPos = s->pos;

        if( s->kind == Statement::ExprStat )
        {
            exprSeq(s->e);
            return;
        }

        switch( s->kind )
        {
        case op_IF:
            emitIf(s);
            break;
        case op_LOOP:
            emitLoop(s);
            break;
        case op_WHILE:
            emitWhile(s);
            break;
        case op_REPEAT:
            emitRepeat(s);
            break;
        case op_SWITCH:
            emitSwitch(s);
            break;
        case op_exit:
            if( loopExits.isEmpty() )
                error("exit outside of a loop", curPos);
            else
                loopExits.back() << emitPendingJump(curPos);
            break;
        case op_label:
            emitLabel(s);
            break;
        case op_goto:
            emitGoto(s);
            break;
        case op_pop:
        case op_free:
            if( require(1,curPos) )
                releaseSlot();
            break;
        case op_ret:
            if( curProc && curProc->getType() )
            {
                if( !require(1,curPos) )
                    break;
                bc.RET(top().slot, 1, pos(curPos));
                releaseSlot();
            }else
                bc.RET(pos(curPos));
            break;
        case op_raise: {
                if( !require(1,curPos) )
                    break;
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("raise", 1, curPos);
                bc.MOV(base + cfs, top().slot, pos(curPos));
                bc.CALL(base, 0, 1, pos(curPos));
                endCall(base, cfs + 1, 1, false, 0, ST_void, curPos);
            } break;
        case op_starg:
        case op_stloc: {
                if( !require(1,curPos) )
                    break;
                const bool arg = s->kind == op_starg;
                const quint8 slot = slotOf(s->id, arg);
                if( arg && isRefParam(s->id) )
                {
                    const int cfs = bc.getCallFrameSize();
                    const int base = ctx.back().buySlots(cfs + 1, true);
                    getField(base, slot, "set", curPos);
                    bc.MOV(base + cfs, top().slot, pos(curPos));
                    bc.CALL(base, 0, 1, pos(curPos));
                    ctx.back().sellSlots(base, cfs + 1);
                    releaseSlot();
                    break;
                }
                if( isBoxed(s->id, arg) )
                    bc.TSETi(top().slot, slot, 1, pos(curPos));
                else
                    bc.MOV(slot, top().slot, pos(curPos));
                releaseSlot();
            } break;
        case op_stvar: {
                if( !require(1,curPos) )
                    break;
                const quint8 tmp = ctx.back().buySlots(1);
                fetchOwner(tmp, s->d, curPos);
                setField(top().slot, tmp, keyOf(s->d), curPos);
                ctx.back().sellSlots(tmp);
                releaseSlot();
            } break;
        case op_stfld: {
                if( !require(2,curPos) )
                    break;
                setField(top().slot, top(1).slot,
                         s->d ? s->d->name : QByteArray(), curPos);
                releaseSlot();
                releaseSlot();
            } break;
        case op_stelem: {
                if( !require(3,curPos) )
                    break;
                bc.TSET(top().slot, top(2).slot, top(1).slot, pos(curPos));
                releaseSlot();
                releaseSlot();
                releaseSlot();
            } break;
        case op_copy: {
                if( !require(2,curPos) )
                    break;
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("copy", 2, curPos);
                bc.MOV(base + cfs, top(1).slot, pos(curPos));
                bc.MOV(base + cfs + 1, top().slot, pos(curPos));
                bc.CALL(base, 0, 2, pos(curPos));
                endCall(base, cfs + 2, 2, false, 0, ST_void, curPos);
            } break;
        case op_transfer: {
                if( !require(1,curPos) )
                    break;
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("transfer", 1, curPos);
                bc.MOV(base + cfs, top().slot, pos(curPos));
                bc.CALL(base, 0, 1, pos(curPos));
                endCall(base, cfs + 1, 1, false, 0, ST_void, curPos);
            } break;
        case op_stind:
            error(QString("the LuaJIT backend does not support '%1'").
                  arg(s_opName[s->kind]), curPos);
            break;
        default:
            error(QString("unexpected statement: %1").arg(s_opName[s->kind]), curPos);
            break;
        }
    }

    quint8 slotOf(quint32 id, bool arg) const
    {
        const QList<quint8>& l = arg ? argSlots : localSlots;
        if( (int)id >= l.size() )
            return 0;
        return l[id];
    }

    bool isBoxed(quint32 id, bool arg) const
    {
        if( arg && isRefParam(id) )
            return false; // a VAR parameter is already a designator
        return arg ? boxedArgs.contains(id) : boxedLocals.contains(id);
    }

    bool isRefParam(quint32 id) const
    {
        // a VAR parameter receives a designator
        if( curProc == 0 )
            return false;
        const DeclList l = curProc->getParams();
        return (int)id < l.size() && l[id]->varParam;
    }

    Type* declTypeOf(quint32 id, bool arg) const
    {
        if( curProc == 0 )
            return 0;
        const DeclList l = arg ? curProc->getParams() : curProc->getLocals();
        if( (int)id >= l.size() )
            return 0;
        return l[id]->getType();
    }

    void emitIf(Statement*& s)
    {
        exprSeq(s->e);
        if( !require(1,curPos) )
            return;
        const quint32 toElse = emitJumpIfFalse(top(), curPos);
        releaseSlot();

        statementSeq(s->body);

        if( s->next && s->next->kind == op_ELSE )
        {
            const quint32 toEnd = emitPendingJump(curPos);
            bc.patch(toElse);
            s = s->next;
            statementSeq(s->body);
            bc.patch(toEnd);
        }else
            bc.patch(toElse);
    }

    void emitLoop(Statement* s)
    {
        loopExits.push_back(QList<quint32>());
        bc.LOOP(ctx.back().pool.d_frameSize, 0, pos(curPos));
        const quint32 loopStart = bc.getCurPc();

        statementSeq(s->body);

        bc.jumpToLoop(loopStart, Lua::JitComposer::currentHighWater(ctx.back().pool),
                      pos(curPos));

        foreach( quint32 pc, loopExits.back() )
            bc.patch(pc);
        loopExits.pop_back();
    }

    void emitWhile(Statement* s)
    {
        loopExits.push_back(QList<quint32>());
        bc.LOOP(ctx.back().pool.d_frameSize, 0, pos(curPos));
        const quint32 loopStart = bc.getCurPc();

        exprSeq(s->e);
        if( !require(1,curPos) )
        {
            loopExits.pop_back();
            return;
        }
        const quint32 outOfLoop = emitJumpIfFalse(top(), curPos);
        releaseSlot();

        statementSeq(s->body);

        bc.jumpToLoop(loopStart, Lua::JitComposer::currentHighWater(ctx.back().pool),
                      pos(curPos));

        bc.patch(outOfLoop);
        foreach( quint32 pc, loopExits.back() )
            bc.patch(pc);
        loopExits.pop_back();
    }

    void emitRepeat(Statement* s)
    {
        loopExits.push_back(QList<quint32>());
        bc.LOOP(ctx.back().pool.d_frameSize, 0, pos(curPos));
        const quint32 loopStart = bc.getCurPc();

        statementSeq(s->body);

        exprSeq(s->e);
        if( !require(1,curPos) )
        {
            loopExits.pop_back();
            return;
        }
        const quint32 outOfLoop = emitJumpIfTrue(top(), curPos);
        releaseSlot();

        bc.jumpToLoop(loopStart, Lua::JitComposer::currentHighWater(ctx.back().pool),
                      pos(curPos));

        bc.patch(outOfLoop);
        foreach( quint32 pc, loopExits.back() )
            bc.patch(pc);
        loopExits.pop_back();
    }

    void emitSwitch(Statement*& s)
    {
        exprSeq(s->e);
        if( !require(1,curPos) )
            return;
        const Val sel = top(); // the selector lives across all cases

        QList<quint32> toEnd;
        while( s->next && s->next->kind == op_CASE )
        {
            s = s->next;
            QList<quint32> toBody;
            QList<quint32> toNext;
            for( int i = 0; s->labels && i < s->labels->size(); i++ )
            {
                const CaseLabel& l = s->labels->at(i);
                const bool last = i == s->labels->size() - 1;
                if( l.first == l.second )
                {
                    bc.ISEQ(sel.slot, QVariant(qlonglong(l.first)), pos(curPos));
                    toBody << emitPendingJump(curPos);
                }else
                {
                    bc.ISLT(sel.slot, constSlot(qlonglong(l.first), curPos), pos(curPos));
                    toNext << emitPendingJump(curPos); // sel < lower bound
                    bc.ISGT(sel.slot, constSlot(qlonglong(l.second), curPos), pos(curPos));
                    toNext << emitPendingJump(curPos); // sel > upper bound
                    toBody << emitPendingJump(curPos);
                }
                if( last )
                    toNext << emitPendingJump(curPos);
            }
            if( s->labels == 0 || s->labels->isEmpty() )
                toNext << emitPendingJump(curPos);

            foreach( quint32 pc, toBody )
                bc.patch(pc);
            statementSeq(s->body);
            toEnd << emitPendingJump(curPos);

            foreach( quint32 pc, toNext )
                bc.patch(pc);
        }
        if( s->next && s->next->kind == op_ELSE )
        {
            s = s->next;
            statementSeq(s->body);
        }
        foreach( quint32 pc, toEnd )
            bc.patch(pc);

        // release the selector
        while( !stack.isEmpty() && stack.back().slot != sel.slot )
            releaseSlot();
        releaseSlot();
    }

    quint8 constSlot(qlonglong v, const RowCol& p)
    {
        const quint8 tmp = ctx.back().buySlots(1);
        bc.KSET(tmp, QVariant(v), pos(p));
        ctx.back().sellSlots(tmp);
        return tmp;
    }

    void emitLabel(Statement* s)
    {
        // the label refers to the next instruction to be emitted
        const QList<quint32> pending = pendingGotos.value(s->name);
        foreach( quint32 pc, pending )
            bc.patch(pc);
        pendingGotos.remove(s->name);
        labelPc.insert(s->name, bc.getCurPc());
    }

    void emitGoto(Statement* s)
    {
        QHash<QByteArray,quint32>::const_iterator i = labelPc.find(s->name);
        if( i != labelPc.end() )
        {
            // backward jump; the target is the instruction after i.value()
            const int target = i.value() + 1;
            const int off = target - bc.getCurPc() - 2;
            if( off < -0x7fff )
                error("the jump distance exceeds the LuaJIT limit", curPos);
            emitJump(off, curPos);
        }else
            pendingGotos[s->name] << emitPendingJump(curPos);
    }


    void exprSeq(Expression* e)
    {
        while( e )
        {
            emitExpression(e);
            e = e->next;
        }
    }

    void emitExpression(Expression* e)
    {
        if( e->pos.isValid() )
            curPos = e->pos;

        switch( e->kind )
        {
        case op_ldc_i4:
        case op_ldc_i8: {
                const quint8 res = ctx.back().buySlots(1);
                bc.KSET(res, QVariant(qlonglong(e->i)), pos(curPos));
                push(res, mdl->getBasicType(e->kind == op_ldc_i4 ?
                                                Type::INT32 : Type::INT64));
            } break;
        case op_ldc_r4:
        case op_ldc_r8: {
                const quint8 res = ctx.back().buySlots(1);
                bc.KSET(res, QVariant(e->f), pos(curPos));
                push(res, mdl->getBasicType(e->kind == op_ldc_r4 ?
                                                Type::FLOAT32 : Type::FLOAT64));
            } break;
        case op_ldnull: {
                const quint8 res = ctx.back().buySlots(1);
                bc.KNIL(res, 1, pos(curPos));
                push(res, 0, ST_nil);
            } break;
        case op_ldstr: {
                const quint8 res = ctx.back().buySlots(1);
                if( e->c && ( e->c->kind == Constant::S || e->c->kind == Constant::B ) )
                    bc.KSET(res, e->c->s, pos(curPos));
                else if( e->c )
                    bc.KSET(res, e->c->toVariant(), pos(curPos));
                else
                    bc.KNIL(res, 1, pos(curPos));
                push(res, 0, ST_ref);
            } break;
        case op_ldarg:
        case op_ldloc: {
                const bool arg = e->kind == op_ldarg;
                if( arg && isRefParam(e->id) )
                {
                    const int cfs = bc.getCallFrameSize();
                    const int base = ctx.back().buySlots(cfs, true);
                    getField(base, slotOf(e->id,true), "get", curPos);
                    bc.CALL(base, 1, 0, pos(curPos));
                    endCall(base, cfs, 0, true, declTypeOf(e->id,true), ST_void, curPos);
                    break;
                }
                const quint8 res = ctx.back().buySlots(1);
                if( isBoxed(e->id, arg) )
                    bc.TGETi(res, slotOf(e->id,arg), 1, pos(curPos));
                else
                    bc.MOV(res, slotOf(e->id,arg), pos(curPos));
                push(res, declTypeOf(e->id,arg));
            } break;
        case op_refarg:
        case op_refloc: {
                const bool arg = e->kind == op_refarg;
                if( arg && isRefParam(e->id) )
                {
                    // the designator is forwarded as it is
                    const quint8 res = ctx.back().buySlots(1);
                    bc.MOV(res, slotOf(e->id,true), pos(curPos));
                    push(res, 0, ST_desig);
                    break;
                }
                const quint8 res = ctx.back().buySlots(1);
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("desig", 2, curPos);
                bc.MOV(base + cfs, slotOf(e->id,arg), pos(curPos));
                bc.KSET(base + cfs + 1, QVariant(1), pos(curPos));
                bc.CALL(base, 1, 2, pos(curPos));
                bc.MOV(res, base, pos(curPos));
                ctx.back().sellSlots(base, cfs + 2);
                push(res, 0, ST_desig);
            } break;
        case op_ldvar: {
                const quint8 res = ctx.back().buySlots(1);
                fetchDecl(res, e->d, curPos);
                push(res, typeOf(e->d));
            } break;
        case op_refvar: {
                const quint8 res = ctx.back().buySlots(1);
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("desig", 2, curPos);
                fetchOwner(base + cfs, e->d, curPos);
                bc.KSET(base + cfs + 1, keyOf(e->d), pos(curPos));
                bc.CALL(base, 1, 2, pos(curPos));
                bc.MOV(res, base, pos(curPos));
                ctx.back().sellSlots(base, cfs + 2);
                push(res, 0, ST_desig);
            } break;
        case op_ldfld: {
                if( !require(1,curPos) )
                    break;
                const quint8 obj = top().slot;
                getField(obj, obj, e->d ? e->d->name : QByteArray(), curPos);
                stack.back() = Val(obj, typeOf(e->d),
                                   typeOf(e->d) ? typeOf(e->d)->stackType() : ST_void);
            } break;
        case op_reffld: {
                if( !require(1,curPos) )
                    break;
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("desig", 2, curPos);
                bc.MOV(base + cfs, top().slot, pos(curPos));
                bc.KSET(base + cfs + 1, e->d ? e->d->name : QByteArray(), pos(curPos));
                bc.CALL(base, 1, 2, pos(curPos));
                endCall(base, cfs + 2, 1, true, 0, ST_desig, curPos);
            } break;
        case op_ldelem: {
                if( !require(2,curPos) )
                    break;
                Type* at = typeOf(e->d);
                Type* et = at ? at->getType() : 0;
                if( et && et->deref()->kind == Type::CHAR )
                {
                    const int cfs = bc.getCallFrameSize();
                    const int base = beginRtCall("chr", 2, curPos);
                    bc.MOV(base + cfs, top(1).slot, pos(curPos));
                    bc.MOV(base + cfs + 1, top().slot, pos(curPos));
                    bc.CALL(base, 1, 2, pos(curPos));
                    endCall(base, cfs + 2, 2, true, et, ST_int32, curPos);
                }else
                {
                    const quint8 arr = top(1).slot;
                    bc.TGET(arr, arr, top().slot, pos(curPos));
                    releaseSlot();
                    stack.back() = Val(arr, et, et ? et->stackType() : ST_void);
                }
            } break;
        case op_refelem: {
                if( !require(2,curPos) )
                    break;
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("desig", 2, curPos);
                bc.MOV(base + cfs, top(1).slot, pos(curPos));
                bc.MOV(base + cfs + 1, top().slot, pos(curPos));
                bc.CALL(base, 1, 2, pos(curPos));
                endCall(base, cfs + 2, 2, true, 0, ST_desig, curPos);
            } break;
        case op_len: {
                if( !require(1,curPos) )
                    break;
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("len", 1, curPos);
                bc.MOV(base + cfs, top().slot, pos(curPos));
                bc.CALL(base, 1, 1, pos(curPos));
                endCall(base, cfs + 1, 1, true, mdl->getBasicType(Type::INT32),
                        ST_int32, curPos);
            } break;
        case op_ldproc: {
                const quint8 res = ctx.back().buySlots(1);
                fetchDecl(res, e->d, curPos);
                push(res, 0, ST_procref);
            } break;
        case op_ldmeth:
        case op_ldvirt: {
                if( !require(1,curPos) )
                    break;
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("bind", 2, curPos);
                if( e->kind == op_ldvirt && e->d )
                    getField(base + cfs, top().slot, e->d->name, curPos);
                else
                    fetchDecl(base + cfs, e->d, curPos);
                bc.MOV(base + cfs + 1, top().slot, pos(curPos));
                bc.CALL(base, 1, 2, pos(curPos));
                endCall(base, cfs + 2, 1, true, 0, ST_methref, curPos);
            } break;
        case op_newobj:
        case op_newobj_um: {
                Type* t = typeOf(e->d);
                if( t == 0 || t->decl == 0 )
                {
                    error("newobj requires a named type", curPos);
                    break;
                }
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("newobj", 2, curPos);
                fetchOwner(base + cfs, t->decl, curPos);
                getField(base + cfs, base + cfs, keyOf(t->decl) + "$p", curPos);
                if( t->kind == Type::Record )
                    fetchTypeObject(base + cfs + 1, t, curPos);
                else
                    bc.KNIL(base + cfs + 1, 1, pos(curPos));
                bc.CALL(base, 1, 2, pos(curPos));
                endCall(base, cfs + 2, 0, true, t, ST_ref, curPos);
            } break;
        case op_newarr:
        case op_newarr_um: {
                if( !require(1,curPos) )
                    break;
                Type* at = typeOf(e->d);
                Type* et = at ? at->getType() : 0;
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("arr", 2, curPos);
                bc.MOV(base + cfs, top().slot, pos(curPos));
                const QVariant init = defaultOf(et);
                if( init.isValid() )
                    bc.KSET(base + cfs + 1, init, pos(curPos));
                else
                    bc.KNIL(base + cfs + 1, 1, pos(curPos));
                bc.CALL(base, 1, 2, pos(curPos));
                endCall(base, cfs + 2, 1, true, at, ST_ref, curPos);
            } break;
        case op_isinst: {
                if( !require(1,curPos) )
                    break;
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("isinst", 2, curPos);
                bc.MOV(base + cfs, top().slot, pos(curPos));
                fetchTypeObject(base + cfs + 1, typeOf(e->d), curPos);
                bc.CALL(base, 1, 2, pos(curPos));
                endCall(base, cfs + 2, 1, true, mdl->getBasicType(Type::INT32),
                        ST_int32, curPos);
            } break;
        case op_castobj: {
                if( !require(1,curPos) )
                    break;
                Type* t = typeOf(e->d);
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("castobj", 2, curPos);
                bc.MOV(base + cfs, top().slot, pos(curPos));
                fetchTypeObject(base + cfs + 1, t, curPos);
                bc.CALL(base, 1, 2, pos(curPos));
                endCall(base, cfs + 2, 1, true, t, ST_ref, curPos);
            } break;
        case op_dup: {
                if( !require(1,curPos) )
                    break;
                const Val v = top();
                const quint8 res = ctx.back().buySlots(1);
                bc.MOV(res, v.slot, pos(curPos));
                push(res, v.t, v.st);
            } break;
        case op_neg: {
                if( !require(1,curPos) )
                    break;
                bc.UNM(top().slot, top().slot, pos(curPos));
            } break;
        case op_abs: {
                if( !require(1,curPos) )
                    break;
                const Val v = top();
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("abs", 1, curPos);
                bc.MOV(base + cfs, v.slot, pos(curPos));
                bc.CALL(base, 1, 1, pos(curPos));
                endCall(base, cfs + 1, 1, true, v.t, v.st, curPos);
            } break;
        case op_not: {
                if( !require(1,curPos) )
                    break;
                const Val v = top();
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall(isBool(v) ? "lnot" : "bnot", 1, curPos);
                bc.MOV(base + cfs, v.slot, pos(curPos));
                bc.CALL(base, 1, 1, pos(curPos));
                endCall(base, cfs + 1, 1, true, v.t, v.st, curPos);
            } break;
        case op_add:
        case op_sub:
        case op_mul:
        case op_div:
        case op_rem:
        case op_div_un:
        case op_rem_un:
            emitArith(e);
            break;
        case op_and:
        case op_or:
        case op_xor:
        case op_shl:
        case op_shr:
        case op_shr_un:
            emitBitop(e);
            break;
        case op_ceq:
        case op_cgt:
        case op_cgt_un:
        case op_clt:
        case op_clt_un:
            emitCompare(e);
            break;
        case op_conv_i1:
        case op_conv_i2:
        case op_conv_i4:
        case op_conv_u1:
        case op_conv_u2:
        case op_conv_u4:
        case op_conv_i8:
        case op_conv_u8: {
                if( !require(1,curPos) )
                    break;
                Type* t = mdl->getBasicType(e->kind == op_conv_i8 || e->kind == op_conv_u8 ?
                                                Type::INT64 : Type::INT32);
                const Val v = top();
                if( v.st == ST_int32 || v.st == ST_int64 )
                    stack.back() = Val(v.slot, t, t->stackType());
                else
                {
                    const int cfs = bc.getCallFrameSize();
                    const int base = beginRtCall("trunc", 1, curPos);
                    bc.MOV(base + cfs, v.slot, pos(curPos));
                    bc.CALL(base, 1, 1, pos(curPos));
                    endCall(base, cfs + 1, 1, true, t, t->stackType(), curPos);
                }
            } break;
        case op_conv_r4:
        case op_conv_r8: {
                if( !require(1,curPos) )
                    break;
                // all Lua numbers are doubles, so this is a no-op
                Type* t = mdl->getBasicType(e->kind == op_conv_r4 ?
                                                Type::FLOAT32 : Type::FLOAT64);
                stack.back() = Val(top().slot, t, t->stackType());
            } break;
        case op_iif:
            emitIif(e);
            break;
        case op_call:
        case op_pcall: {
                if( e->d == 0 )
                {
                    error("call without procedure", curPos);
                    break;
                }
                emitCall(e->d, 0, e->d->getParams().size(), false,
                         e->kind == op_pcall, e->d->getType());
            } break;
        case op_callinst:
        case op_callvirt: {
                if( e->d == 0 )
                {
                    error("call without procedure", curPos);
                    break;
                }
                // the receiver is the first argument; callinst calls the given
                // procedure, callvirt the one bound to the dynamic type
                emitCall(e->d, 0, e->d->getParams().size(), false, false, e->d->getType(),
                         e->kind == op_callvirt);
            } break;
        case op_calli:
        case op_pcalli:
        case op_callmi: {
                Type* t = typeOf(e->d);
                if( t == 0 )
                {
                    error("call without procedure type", curPos);
                    break;
                }
                emitCall(0, t, t->getParams().size(), true,
                         e->kind == op_pcalli, t->getType());
            } break;
        case op_curtask: {
                const int cfs = bc.getCallFrameSize();
                const int base = beginRtCall("curtask", 0, curPos);
                bc.CALL(base, 1, 0, pos(curPos));
                endCall(base, cfs, 0, true, mdl->getBasicType(Type::TASK),
                        ST_task, curPos);
            } break;
        case op_newtask:
        case op_taskdone: {
                if( !require(1,curPos) )
                    break;
                const int cfs = bc.getCallFrameSize();
                const bool done = e->kind == op_taskdone;
                const int base = beginRtCall(done ? "taskdone" : "newtask", 1, curPos);
                bc.MOV(base + cfs, top().slot, pos(curPos));
                bc.CALL(base, 1, 1, pos(curPos));
                endCall(base, cfs + 1, 1, true,
                        mdl->getBasicType(done ? Type::BOOL : Type::TASK),
                        done ? ST_int32 : ST_task, curPos);
            } break;
        case op_caddr:
        case op_ldind:
        case op_sizeof:
            error(QString("the LuaJIT backend does not support '%1'").
                  arg(s_opName[e->kind]), curPos);
            break;
        default:
            error(QString("unexpected expression: %1").arg(s_opName[e->kind]), curPos);
            break;
        }
    }

    void emitArith(Expression* e)
    {
        if( !require(2,curPos) )
            return;
        const Val b = top();
        const Val a = top(1);
        const bool integer = a.st == ST_int32 || a.st == ST_int64;
        switch( e->kind )
        {
        case op_add:
            bc.ADD(a.slot, a.slot, b.slot, pos(curPos));
            break;
        case op_sub:
            bc.SUB(a.slot, a.slot, b.slot, pos(curPos));
            break;
        case op_mul:
            bc.MUL(a.slot, a.slot, b.slot, pos(curPos));
            break;
        case op_div:
        case op_div_un:
            if( integer )
            {
                emitBinRtCall("idiv", a, b);
                return;
            }
            bc.DIV(a.slot, a.slot, b.slot, pos(curPos));
            break;
        default:
            emitBinRtCall("irem", a, b);
            return;
        }
        releaseSlot(); // b
    }

    void emitBitop(Expression* e)
    {
        if( !require(2,curPos) )
            return;
        const char* fn = 0;
        switch( e->kind )
        {
        case op_and:
            fn = "band";
            break;
        case op_or:
            fn = "bor";
            break;
        case op_xor:
            fn = "bxor";
            break;
        case op_shl:
            fn = "shl";
            break;
        case op_shr:
            fn = "shr";
            break;
        default:
            fn = "shr_un";
            break;
        }
        emitBinRtCall(fn, top(1), top());
    }

    void emitBinRtCall(const QByteArray& fn, const Val& a, const Val& b)
    {
        const int cfs = bc.getCallFrameSize();
        const int base = beginRtCall(fn, 2, curPos);
        bc.MOV(base + cfs, a.slot, pos(curPos));
        bc.MOV(base + cfs + 1, b.slot, pos(curPos));
        bc.CALL(base, 1, 2, pos(curPos));
        endCall(base, cfs + 2, 2, true, a.t, a.st, curPos);
    }

    void emitCompare(Expression* e)
    {
        if( !require(2,curPos) )
            return;
        const Val b = top();
        const Val a = top(1);
        switch( e->kind )
        {
        case op_ceq:
            bc.ISEQ(a.slot, b.slot, pos(curPos));
            break;
        case op_cgt:
        case op_cgt_un:
            bc.ISGT(a.slot, b.slot, pos(curPos));
            break;
        default:
            bc.ISLT(a.slot, b.slot, pos(curPos));
            break;
        }
        const quint32 toTrue = emitPendingJump(curPos);
        bc.KSET(a.slot, QVariant(0), pos(curPos));
        const quint32 toEnd = emitPendingJump(curPos);
        bc.patch(toTrue);
        bc.KSET(a.slot, QVariant(1), pos(curPos));
        bc.patch(toEnd);

        releaseSlot(); // b
        stack.back() = Val(a.slot, mdl->getBasicType(Type::BOOL), ST_int32);
    }

    void emitIif(Expression* e)
    {
        Expression* if_ = e->e;
        if( if_ == 0 || if_->next == 0 || if_->next->next == 0 )
        {
            error("incomplete conditional expression", curPos);
            return;
        }
        const quint8 res = ctx.back().buySlots(1);

        exprSeq(if_->e);
        if( !require(1,curPos) )
            return;
        const quint32 toElse = emitJumpIfFalse(top(), curPos);
        releaseSlot();

        exprSeq(if_->next->e);
        if( !require(1,curPos) )
            return;
        const Val a = top();
        bc.MOV(res, a.slot, pos(curPos));
        releaseSlot();
        const quint32 toEnd = emitPendingJump(curPos);

        bc.patch(toElse);
        exprSeq(if_->next->next->e);
        if( !require(1,curPos) )
            return;
        bc.MOV(res, top().slot, pos(curPos));
        releaseSlot();
        bc.patch(toEnd);

        push(res, a.t, a.st);
    }

    void emitCall(Declaration* proc, Type* procType, int nargs, bool indirect, bool prot, Type* ret, bool virt = false)
    {
        const int nOperands = nargs + ( indirect ? 1 : 0 );
        if( !require(nOperands,curPos) )
            return;

        const int cfs = bc.getCallFrameSize();
        const int extra = prot ? 1 : 0; // A.pcall(f, ...)
        const int base = ctx.back().buySlots(cfs + extra + nargs, true);

        if( prot )
        {
            fetchRuntime(base, curPos);
            getField(base, base, "pcall", curPos);
        }

        // the called procedure goes to the base of the call frame
        // or to the first argument of A.pcall in the protected case
        const int procAt = prot ? base + cfs : base;
        // the arguments are the lowest of the operands
        // the procedure value is on top of them
        const int argBase = base + cfs + extra;
        const int argStart = stack.size() - nOperands;
        if( indirect )
            bc.MOV(procAt, top().slot, pos(curPos));
        else if( virt && nargs > 0 )
            // the procedure bound to the dynamic type of the receiver
            getField(procAt, stack[argStart].slot, proc->name, curPos);
        else
            fetchDecl(procAt, proc, curPos);

        for( int i = 0; i < nargs; i++ )
            bc.MOV(argBase + i, stack[argStart+i].slot, pos(curPos));

        const bool wantResult = prot || ret != 0;
        bc.CALL(base, wantResult ? 1 : 0, extra + nargs, pos(curPos));

        endCall(base, cfs + extra + nargs, nOperands, wantResult,
                prot ? 0 : ret, prot ? ST_ref : ST_void, curPos);
    }
};

LjGen::LjGen(AstModel* mdl)
{
    Q_ASSERT( mdl );
    imp = new Imp();
    imp->mdl = mdl;
}

LjGen::~LjGen()
{
    delete imp;
}

bool LjGen::generate(Declaration* module, QIODevice* out, bool stripped)
{
    Q_ASSERT( module && module->kind == Declaration::Module && out );
    imp->bc.clear();
    imp->bc.setStripped(stripped);
    try
    {
        return imp->translate(module, out);
    }catch( const Imp::NoMoreFreeSlots& )
    {
        Error e;
        e.msg = "too many local slots required by the module";
        e.where = module->name;
        e.pc = imp->curPos.d_row;
        imp->errors << e;
        return false;
    }
}

QByteArray LjGen::luaName(const QByteArray& airName)
{
    return Imp::sanitize(airName);
}

const QList<LjGen::Error>& LjGen::getErrors() const
{
    return imp->errors;
}
