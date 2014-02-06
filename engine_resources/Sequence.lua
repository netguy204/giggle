local oo = require 'oo'
local util = require 'util'

local all = {}
local Sequence = oo.class(oo.Object)
function Sequence:init()
   self.queue = {}
   self.running = false
   table.insert(all, self)
end

function Sequence:terminate()
   util.table_remove(all, self)
   self.running = false
   self.comp:delete_me(1)
end

function Sequence.terminate_all()
   for ii=1,#all do
      all[ii]:terminate()
   end
end

local function saturate(x)
   if x > 1 then
      return 1
   elseif x < 0 then
      return 0
   else
      return x
   end
end

function Sequence.lerp(start, stop, dt, rdt)
   local s = (1 - rdt / dt)
   local dx = stop - start
   return start + dx * s
end

function Sequence.smoothstep(start, stop, dt, rdt)
   local s = (1 - rdt / dt)
   local dx = stop - start
   return start + dx * (s * s * (3 - 2 * s))
end

function Sequence.smootherstep(start, stop, dt, rdt)
   local s = (1 - rdt / dt)
   local dx = stop - start
   return start + dx * (s * s * s * (s * (s * 6 - 15) + 10))
end

function Sequence:transition(obj, method, start, stop, dt, interp)
   local interp_between = function(item)
      item.rdt = item.rdt - world:dt()

      if item.rdt <= 0 then
         obj[method](obj, item.stop)
         return false
      else
         local x = interp(item.start, item.stop, item.dt, item.rdt)
         obj[method](obj, x)
         return true
      end
   end

   self:next(interp_between, {start=start, stop=stop, dt=dt, rdt=dt})
end

function Sequence:animate_between(go, start, stop, dt, method)
   method = method or Sequence.smootherstep
   self:transition(go, 'pos', start, stop, dt, method)
end

function Sequence:rotate_between(go, start, stop, dt, method)
   method = method or Sequence.smootherstep
   self:transition(go, 'angle', start, stop, dt, method)
end

function Sequence:wait(dt)
   local wait = function(item)
      item.rdt = item.rdt - world:dt()
      return item.rdt > 0
   end
   self:next(wait, {rdt = dt})
end

function Sequence:next(fn, ...)
   local lfn = function(item)
      return fn(table.unpack(item))
   end
   table.insert(self.queue, {fn=lfn, arg={...}})
end

function Sequence:start()
   if util.empty(self.queue) or self.running then
      return
   end

   self.running = true
   local comp

   local thread = function()
      while true do
         local item = self.queue[1]
         if not item.fn(item.arg) then
            table.remove(self.queue, 1)
            if util.empty(self.queue) then
               self:terminate()
               return
            end
         else
            return
         end
      end
   end

   comp = stage:add_component('CScripted', {update_thread=util.fthread(thread)})
   self.comp = comp
end

return Sequence
