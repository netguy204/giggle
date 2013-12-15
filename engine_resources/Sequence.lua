local oo = require 'oo'
local util = require 'util'

local Sequence = oo.class(oo.Object)
function Sequence:init()
   self.queue = {}
   self.running = false
end

local function lerp(start, stop, dt, rdt)
   local s = (1 - rdt / dt)
   local dx = stop - start
   return start + dx * s
end

function Sequence:animate_between(go, start, stop, dt)
   local animate_between = function(item)
      local x = lerp(item.start, item.stop, item.dt, item.rdt)
      item.go:pos(x)
      item.rdt = item.rdt - world:dt()
      if item.rdt <= 0 then
         item.go:pos(item.stop)
         return false
      else
         return true
      end
   end
   self:next(animate_between, {go=go, start=start, stop=stop, dt=dt, rdt=dt})
end

function Sequence:rotate_between(go, start, stop, dt)
   local rotate_between = function(item)
      local a = lerp(item.start, item.stop, item.dt, item.rdt)
      item.go:angle(a)
      item.rdt = item.rdt - world:dt()
      if item.rdt <= 0 then
         item.go:angle(stop)
         return false
      else
         return true
      end
   end
   self:next(rotate_between, {go=go, start=start, stop=stop, dt=dt, rdt=dt})
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
      local item = self.queue[1]
      if not item.fn(item.arg) then
         table.remove(self.queue, 1)
         if util.empty(self.queue) then
            comp:delete_me(1)
            self.running = false
            return
         end
      end
   end

   comp = stage:add_component('CScripted', {update_thread=util.fthread(thread)})
end

return Sequence
