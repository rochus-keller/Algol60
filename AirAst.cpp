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

#include <Algol60/AirAst.h>
#include <Algol60/AirRenderer.h>
#include <QtDebug>
using namespace Air;

static const char* s_stackTypeName[] = {
    "void", "int32", "int64", "float32", "float64",
    "ref", "procref", "methref", "task", "desig", "nil"
};

const char* Air::stackTypeName(quint8 t)
{
    if( t > ST_nil )
        return "?";
    return s_stackTypeName[t];
}

Node::~Node()
{
    if( type && ownstype )
        delete type;
}

void Node::setType(Type* t)
{
    if( t )
        Q_ASSERT( t != this );
    if( type && ownstype )
        delete type;
    type = t;
    ownstype = false;
    if( t && !t->owned )
    {
        ownstype = true;
        t->owned = true;
        if( meta == D && t->decl == 0 )
            t->decl = static_cast<Declaration*>(this);
    }
}

Type::~Type()
{
    if( kind == NameRef && quali )
        delete quali;
    foreach( Declaration* d, subs )
        delete d;
}

quint8 Type::stackType() const
{
    Type* t = deref();
    switch( t->kind )
    {
    case BOOL:
    case CHAR:
    case INT8:
    case INT16:
    case INT32:
    case UINT8:
    case UINT16:
    case UINT32:
        return ST_int32;
    case INT64:
    case UINT64:
        return ST_int64;
    case FLOAT32:
        return ST_float32;
    case FLOAT64:
        return ST_float64;
    case TASK:
        return ST_task;
    case ANYREC:
    case Pointer:
    case Array:
    case Struct:
    case Record:
    case CStruct:
    case CUnion:
    case CArray:
    case CPointer:
        return ST_ref;
    case Proc:
        return t->typebound ? ST_methref : ST_procref;
    default:
        return ST_void;
    }
}

Type* Type::deref() const
{
    if( kind == NameRef && type )
        return type->deref();
    else
        return const_cast<Type*>(this);
}

Declaration* Type::findSubByName(const QByteArray& name, bool recursive) const
{
    if( name.isEmpty() )
        return 0;
    for( int i = 0; i < subs.size(); i++ )
    {
        if( subs[i]->name == name )
            return subs[i];
    }
    if( recursive && getType() )
        return getType()->deref()->findSubByName(name, recursive);
    return 0;
}

DeclList Type::getFieldList(bool recursive) const
{
    DeclList res;
    if( recursive && getType() )
        res = getType()->deref()->getFieldList(recursive);
    foreach( Declaration* d, subs )
    {
        if( d->kind == Declaration::Field )
            res << d;
    }
    return res;
}

DeclList Type::getParams() const
{
    DeclList res;
    foreach( Declaration* d, subs )
    {
        if( d->kind == Declaration::ParamDecl )
            res << d;
    }
    return res;
}

bool Type::extends(const Type* base) const
{
    if( base == 0 )
        return false;
    const Type* b = base->deref();
    const Type* t = deref();
    if( t == b )
        return true;
    if( b->kind == ANYREC )
        return t->isRecord();
    if( t->kind == Pointer && b->kind == Pointer )
    {
        Type* t1 = t->getType();
        Type* b1 = b->getType();
        if( t1 == 0 || b1 == 0 )
            return false;
        return t1->extends(b1);
    }
    while( t )
    {
        if( t == b )
            return true;
        t = t->getType() ? t->getType()->deref() : 0;
    }
    return false;
}

Quali Type::toQuali() const
{
    if( kind == NameRef )
    {
        if( quali && !quali->first.isEmpty() )
            return *quali;
        if( type )
            return type->toQuali();
        if( quali )
            return *quali;
    }
    if( decl )
        return decl->toQuali();
    else
        return Quali();
}

QVariant Constant::toVariant() const
{
    switch( kind )
    {
    case I:
        return i;
    case D:
        return d;
    case S:
        return QString::fromUtf8(s);
    case B:
        return s;
    default:
        return QVariant();
    }
}

Constant* Constant::fromVariant(const QVariant& v)
{
    Constant* res = new Constant();
    switch( v.type() )
    {
    case QVariant::ByteArray:
        res->kind = B;
        res->s = v.toByteArray();
        break;
    case QVariant::String:
        res->kind = S;
        res->s = v.toString().toUtf8();
        break;
    case QVariant::Double:
        res->kind = D;
        res->d = v.toDouble();
        break;
    case QVariant::Bool:
    case QVariant::Int:
    case QVariant::UInt:
    case QVariant::LongLong:
    case QVariant::ULongLong:
        res->kind = I;
        res->i = v.toLongLong();
        break;
    default:
        res->kind = Invalid;
        break;
    }
    return res;
}

Declaration::~Declaration()
{
    if( next )
        delete next;
    if( subs )
        delete subs;
    if( body )
        delete body;
    switch( kind )
    {
    case ConstDecl:
        if( c )
            delete c;
        break;
    case Procedure:
        if( pd )
            delete pd;
        break;
    case Module:
        if( md )
            delete md;
        break;
    case Import:
        if( stub && imported )
            delete imported;
        break;
    default:
        break;
    }
}

void Declaration::appendSub(Declaration* d)
{
    Q_ASSERT( d );
    d->outer = this;
    if( subs == 0 )
        subs = d;
    else
    {
        Declaration* last = subs;
        while( last->next )
            last = last->next;
        last->next = d;
    }
}

Declaration* Declaration::findSubByName(const QByteArray& name) const
{
    Declaration* d = subs;
    while( d )
    {
        if( d->name == name )
            return d;
        d = d->next;
    }
    return 0;
}

DeclList Declaration::getParams() const
{
    DeclList res;
    Declaration* d = subs;
    while( d )
    {
        if( d->kind == ParamDecl )
            res << d;
        d = d->next;
    }
    return res;
}

DeclList Declaration::getLocals() const
{
    DeclList res;
    Declaration* d = subs;
    while( d )
    {
        if( d->kind == LocalDecl )
            res << d;
        d = d->next;
    }
    return res;
}

int Declaration::indexOf(Declaration* sub) const
{
    int i = 0;
    Declaration* d = subs;
    while( d )
    {
        if( d == sub )
            return i;
        i++;
        d = d->next;
    }
    return -1;
}

Declaration* Declaration::getModule() const
{
    Declaration* d = const_cast<Declaration*>(this);
    while( d )
    {
        if( d->kind == Module )
            return d;
        d = d->outer;
    }
    return 0;
}

ProcedureData* Declaration::getPd()
{
    Q_ASSERT( kind == Procedure );
    if( pd == 0 )
        pd = new ProcedureData();
    return pd;
}

ModuleData* Declaration::getMd()
{
    Q_ASSERT( kind == Module );
    if( md == 0 )
        md = new ModuleData();
    return md;
}

QByteArray Declaration::toPath() const
{
    QByteArray res = name;
    if( outer )
    {
        if( outer->kind == Module )
            res = "!" + res;
        else if( outer->kind != NoMode )
            res = "." + res;
        return outer->toPath() + res;
    }
    return res;
}

Quali Declaration::toQuali() const
{
    Quali res;
    if( kind == Module )
    {
        res.first = name;
        return res;
    }
    res.second = name;
    Declaration* m = getModule();
    if( m )
        res.first = m->name;
    return res;
}

Expression::~Expression()
{
    if( e )
        delete e;
    if( next )
        delete next;
    if( kind == op_ldstr && c )
        delete c;
}

void Expression::append(Expression* e)
{
    Q_ASSERT( e );
    Expression* last = this;
    while( last->next )
        last = last->next;
    last->next = e;
}

Statement::~Statement()
{
    if( kind == op_CASE && labels )
        delete labels;
    if( body )
        delete body;
    if( e )
        delete e;
    if( next )
        delete next;
}

void Statement::append(Statement* s)
{
    Q_ASSERT( s );
    Statement* last = this;
    while( last->next )
        last = last->next;
    last->next = s;
}

AstModel::AstModel()
{
    globals.kind = Declaration::Module;
    for( int i = 0; i < Type::MaxBasicType; i++ )
        basicTypes[i] = 0;
    for( int i = Type::BOOL; i < Type::MaxBasicType; i++ )
    {
        Type* t = new Type();
        t->kind = (Type::Kind)i;
        Declaration* d = new Declaration();
        d->kind = Declaration::TypeDecl;
        d->name = basicTypeName(i);
        d->setType(t);
        globals.appendSub(d);
        d->outer = 0; // the basic types are not qualified by a module
        basicTypes[i] = t;
    }
}

AstModel::~AstModel()
{
    clear();
}

void AstModel::clear()
{
    foreach( Declaration* m, modules )
        delete m;
    modules.clear();
}

bool AstModel::addModule(Declaration* m)
{
    Q_ASSERT( m && m->kind == Declaration::Module );
    if( findModuleByName(m->name) )
        return false;
    modules << m;
    return true;
}

Declaration* AstModel::findModuleByName(const QByteArray& name) const
{
    foreach( Declaration* m, modules )
    {
        if( m->name == name )
            return m;
    }
    return 0;
}

Type* AstModel::getBasicType(quint8 k) const
{
    if( k == 0 || k >= Type::MaxBasicType )
        return 0;
    return basicTypes[k];
}

Declaration* AstModel::resolve(const Quali& q) const
{
    if( q.second.isEmpty() )
        return 0;
    if( q.first.isEmpty() )
    {
        Declaration* d = globals.findSubByName(q.second);
        if( d )
            return d;
        return 0;
    }
    Declaration* m = findModuleByName(q.first);
    if( m == 0 )
        return 0;
    return m->findSubByName(q.second);
}

const char* AstModel::basicTypeName(quint8 k)
{
    return EmiTypes::basicName(k); // Type::Kind and EmiTypes::Basic are numerically equal for basic types
}

quint8 AstModel::basicTypeKind(const QByteArray& name)
{
    for( int i = Type::BOOL; i < Type::MaxBasicType; i++ )
    {
        if( name == basicTypeName(i) )
            return i;
    }
    return Type::Undefined;
}
