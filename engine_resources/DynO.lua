local oo = require 'oo'
local Registry = require 'Registry'
local constant = require 'constant'
local util = require 'util'

local DynO = oo.class(oo.Object)

-- for ease of use, we make our own registry. This could be replaced
-- with a game global registry by the user if desired.
DynO.reg = Registry()

-- class method for the DynO registry
function DynO.find(go)
   return DynO.reg:find(go)
end

function DynO:init(pos)
   self._go = world:create_go()
   self.alive = true
   DynO.reg:register(self._go, self)

   self._go:pos(pos)
   self._go:body_type(constant.DYNAMIC)
   local update = util.fthread(self:bind('update'))
   local message = util.fthread(self:bind('message'))
   self.scripted = self._go:add_component('CScripted', {update_thread=update,
                                                       message_thread=message})
end

function DynO:go()
   if self.alive then
      return self._go
   end
end

function DynO:message()
   local go = self._go
   local msgs = { go:has_message(constant.COLLIDING) }
   for ii, msg in ipairs(msgs) do
      local obj = DynO.find(msg.source)
      if obj then
         self:colliding_with(obj)
      end
   end
end

function DynO:colliding_with(obj)
   -- pass
end

function DynO:add_sensor(parms)
   if not self.sensors then
      self.sensors = {}
   end
   table.insert(self.sensors, self._go:add_component('CSensor', parms))
end

function DynO:add_collider(parms)
   if not self.colliders then
      self.colliders = {}
   end
   table.insert(self.colliders, self._go:add_component('CCollidable', parms))
end

function DynO:terminate()
   self._go:delete_me(1)
   DynO.reg:unregister(self._go)
   self.alive = false
end

return DynO
