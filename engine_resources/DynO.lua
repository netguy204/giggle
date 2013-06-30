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
   self.go = world:create_go()
   DynO.reg:register(self.go, self)

   self.go:pos(pos)
   self.go:body_type(constant.DYNAMIC)
   self.scripted = self.go:add_component('CScripted', {update_thread=util.fthread(self:bind('update')),
                                                       message_thread=util.fthread(self:bind('message'))})
end

function DynO:message()
   local go = self.go
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
   table.insert(self.sensors, self.go:add_component('CSensor', parms))
end

function DynO:add_collider(parms)
   if not self.colliders then
      self.colliders = {}
   end
   table.insert(self.colliders, self.go:add_component('CCollidable', parms))
end

function DynO:terminate()
   self.go:delete_me(1)
   DynO.reg:unregister(self.go)
end

return DynO
