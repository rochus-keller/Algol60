-- the extern procedures declared in Algol60Rt.air.
-- Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>, GPL 2 or 3.

local A = require 'AirRt'

local R = {}

R.Transfer = A.rectype('Algol60Rt!Transfer', nil)

function R.mkxfer(frame, label)
   return setmetatable({ frame = frame, label = label }, R.Transfer)
end

local lineLength = 80
local col = 0

local function put(s)
   if col > 0 and col + #s > lineLength then
      io.write("\n")
      col = 0
   end
   io.write(s)
   col = col + #s
end

function R.stdchannel() return 1 end

function R.outinteger(ch, n)
   put(string.format("%d ", n))
end

function R.outreal(ch, x)
   put(string.format("%.12g ", x))
end

function R.outstring(ch, s)
   for i = 1, #s do
      local c = s:sub(i,i)
      if c == "\n" then
         io.write("\n")
         col = 0
      else
         io.write(c)
         col = col + 1
      end
   end
end

function R.outchar(ch, s, i)
   local c = s:sub(i,i)
   if c == "\n" then
      io.write("\n")
      col = 0
   else
      io.write(c)
      col = col + 1
   end
end

function R.outterminator(ch)
   io.write("\n")
   col = 0
end

local pending = ""

local function token()
   while true do
      pending = pending:gsub("^%s+", "")
      if #pending > 0 then break end
      local l = io.read("l")
      if l == nil then return nil end
      pending = l
   end
   local t = pending:match("^%S+")
   pending = pending:sub(#t+1)
   return t
end

function R.ininteger(ch, d)
   local t = token()
   d.set(math.floor(tonumber(t) or 0))
end

function R.inreal(ch, d)
   local t = token()
   d.set(tonumber(t) or 0.0)
end

function R.inchar(ch, s, d)
   while #pending == 0 do
      local l = io.read("l")
      if l == nil then d.set(0) return end
      pending = l
   end
   local c = pending:sub(1,1)
   pending = pending:sub(2)
   local i = s:find(c, 1, true)
   d.set(i or 0)
end

function R.sqrt(x) return math.sqrt(x) end
function R.sin(x) return math.sin(x) end
function R.cos(x) return math.cos(x) end
function R.arctan(x) return math.atan(x) end
function R.ln(x) return math.log(x) end
function R.exp(x) return math.exp(x) end
function R.entier(x) return math.floor(x) end
function R.sign(x) if x > 0 then return 1 elseif x < 0 then return -1 else return 0 end end
function R.round(x) return math.floor(x + 0.5) end
-- exponentiation of the Report 3.3.4.3; the undefined cases are a fault
function R.expi(a, b)
   if b <= 0 and (b < 0 or a == 0) then R.fault("expi undefined", b) end
   local r = 1
   for i = 1, b do r = r * a end
   return r
end

function R.expn(a, b)
   if a == 0 and b == 0 then R.fault("expn undefined", b) end
   local r = 1.0
   for i = 1, (b < 0 and -b or b) do r = r * a end
   if b < 0 then return 1.0 / r end
   return r
end

function R.expr(a, b)
   if a < 0 or (a == 0 and b <= 0) then R.fault("expr undefined", b) end
   if a == 0 then return 0.0 end
   return math.exp(b * math.log(a))
end

function R.maxreal() return 1.7976931348623157e308 end
function R.minreal() return 2.2250738585072014e-308 end
function R.epsilon() return 2.220446049250313e-16 end
function R.maxint() return 2147483647 end
function R.length(s) return #s end

function R.stop()
   R.finish()
   os.exit(0)
end

local rowColBits = 12

local function row(line)
   if line == nil or line <= 0 then return 0 end
   local r = math.floor(line / 2^rowColBits)
   if r == 0 then return line end
   return r
end

local function basename(path)
   return path:gsub("^@", ""):match("[^/\\]*$")
end

local function frame(i)
   -- the Algol procedures of a module are the functions of its chunk
   -- the runtime itself and the Lua dispatch helpers are not part of the Algol call chain
   if i.func == nil then return nil end
   local name = A.procname(i.func)
   if name == nil then return nil end -- not a procedure of a generated module
   name = name:match("[^.]*$")  -- a procedure bound to a frame record
   local line = string.format(", file %s, line %d",
                              basename(i.source), row(i.currentline))
   if name:match("%$init$") then
      return "main program" .. line
   end
   if name:match("%$") then return nil end -- a generated thunk or switch procedure
   return "procedure " .. name .. line
end

function R.traceback()
   local res = {}
   local l = 2
   while true do
      local i = debug.getinfo(l, "Slf")
      if i == nil then break end
      local f = frame(i)
      if f ~= nil then
         res[#res+1] = f
         if i.what == "main" then break end
      end
      l = l + 1
   end
   return res
end

function R.faultmsg(s)
   R.fault(s)
end

function R.fault(s, x)
   R.finish()
   if x == nil then
      io.write(string.format("fault: %s\n", s))
   else
      io.write(string.format("fault: %s %.12g\n", s, x))
   end
   local tb = R.traceback()
   for i = 1, #tb do
      io.write(tb[i], "\n")
   end
   io.stdout:flush()
   os.exit(1)
end

function R.finish()
   if col > 0 then
      io.write("\n")
      col = 0
   end
   io.stdout:flush()
end

return R
