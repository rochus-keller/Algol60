-- Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>, GPL 2 or 3.

local A = {}

local floor = math.floor
local abs = math.abs

-- record types; a record instance carries its type as its metatable
function A.rectype(name, base)
   local t = { airname = name, airbase = base }
   t.__index = t
   -- bound procedures of the base type are found by the __index chain
   if base ~= nil then setmetatable(t, { __index = base }) end
   return t
end

function A.isinst(obj, T)
   if obj == nil or T == nil then return 0 end
   local t = getmetatable(obj)
   while t do
      if t == T then return 1 end
      t = t.airbase
   end
   return 0
end

function A.castobj(obj, T)
   if obj == nil then return nil end
   if A.isinst(obj, T) == 0 then
      error("invalid type cast", 0)
   end
   return obj
end

function A.newobj(proto, T)
   -- creates an instance of a record or struct type
   -- proto is a table with all fields set to their default value, T the record type or nil for a struct
   local o = {}
   for k, v in pairs(proto) do o[k] = v end
   if T ~= nil then setmetatable(o, T) end
   return o
end

function A.desig(t, k)
   -- a designator refers to the variable t[k]
   return { get = function() return t[k] end,
            set = function(v) t[k] = v end }
end

function A.raise(e) error(e, 0) end

-- boolean values are the integers 0 and 1
function A.lnot(x) if x == 0 then return 1 else return 0 end end

function A.bind(f, self)
   return function(...) return f(self, ...) end
end

-- arrays are zero based tables with the length in the field n
function A.arr(n, init)
   local a = { n = n }
   for i = 0, n-1 do a[i] = init end
   return a
end

function A.len(x)
   if type(x) == "string" then return #x end
   if x == nil then error("dereferencing nil", 0) end
   return x.n or 0
end

function A.chr(s, i)
   if type(s) == "string" then return s:byte(i+1) or 0 end
   return s[i]
end

function A.copy(dst, src)
   if dst == nil or src == nil then error("dereferencing nil", 0) end
   for k in pairs(dst) do dst[k] = nil end
   for k, v in pairs(src) do dst[k] = v end
   if src.n then dst.n = src.n end
end

function A.abs(x) return abs(x) end

function A.idiv(a, b)
   if b == 0 then error("division by zero", 0) end
   local q = a / b
   if q >= 0 then return floor(q) else return -floor(-q) end
end

function A.irem(a, b)
   if b == 0 then error("division by zero", 0) end
   return a - A.idiv(a, b) * b
end

function A.trunc(x)
   if x >= 0 then return floor(x) else return -floor(-x) end
end

function A.flt(x) return x + 0.0 end

-- bitwise operations; on non-integers AIR would be invalid anyway
local band, bor, bxor, bnot, shl, shr
if bit then -- LuaJIT
   band, bor, bxor, bnot = bit.band, bit.bor, bit.bxor, bit.bnot
   shl, shr = bit.lshift, bit.rshift
else
   local f = load or loadstring
   local ops = f([[
      return function(a,b) return a & b end, function(a,b) return a | b end,
             function(a,b) return a ~ b end, function(a) return ~a end,
             function(a,b) return a << b end, function(a,b) return a >> b end
   ]])
   band, bor, bxor, bnot, shl, shr = ops()
end
A.band, A.bor, A.bxor, A.bnot = band, bor, bxor, bnot
A.shl, A.shr, A.shr_un = shl, shr, shr

-- the module tables of the loaded modules
local modules = {}

function A.regmod(m)
   modules[#modules+1] = m
end

function A.procname(f)
   -- the declared name of a procedure, or nil if f is not a module procedure
   for i = 1, #modules do
      for k, v in pairs(modules[i]) do
         if v == f then return k end
      end
   end
   return nil
end

local maintask = { main = true }
local current = maintask

function A.curtask() return current end

function A.newtask(f)
   return { co = coroutine.create(f) }
end

function A.taskdone(t)
   if t == nil or t.main then return 0 end
   if t.done or coroutine.status(t.co) == "dead" then return 1 end
   return 0
end

local function dispatch(target)
   while target ~= nil and target ~= maintask do
      if A.taskdone(target) == 1 then
         error("transfer to a terminated task", 0)
      end
      current = target
      local ok, req = coroutine.resume(target.co)
      if not ok then
         current = maintask
         error(req, 0)
      end
      if coroutine.status(target.co) == "dead" then
         target.done = true
         -- the task terminated, control goes to the task which resumed it
         target = target.resumer or maintask
      else
         target = req
      end
   end
   current = maintask
end

function A.transfer(t)
   if t == nil then error("transfer to nil", 0) end
   if t == current then return end
   t.resumer = current
   if current == maintask then
      dispatch(t)
   else
      coroutine.yield(t)
   end
end

function A.pcall(f, ...)
   -- returns the exception object, or nil if the call returned normally
   local ok, err = pcall(f, ...)
   if ok then return nil end
   return err
end

return A
