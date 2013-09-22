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

function DynO.with_all(fn)
   for key, obj in pairs(DynO.reg.db) do
      fn(obj)
   end
end

function DynO.terminate_all(kind)
   local fn = function(obj)
      if (not kind) or obj:is_a(kind) then
         obj:terminate()
      end
   end
   DynO.with_all(fn)
end

function DynO.with_all_of_type(fn, t)
   for key, obj in pairs(DynO.reg.db) do
      if obj:is_a(t) then
         fn(obj)
      end
   end
end

function DynO.find_all_of_type(t)
   local result = {}
   local fn = function(obj)
      table.insert(result, obj)
   end
   DynO.with_all_of_type(fn, t)
   return result
end

function DynO:init(world, pos)
   self._go = world:create_go()

   self.alive = true
   DynO.reg:register(self._go, self)

   self._go:pos(pos)
   self._go:body_type(constant.DYNAMIC)
   local params = {}

   if self.update then
      params.update_thread = util.fthread(self:bind('update'))
   end
   if self.started_colliding_with or self.stopped_colliding_with or self.message ~= DynO.message then
      params.message_thread = util.fthread(self:bind('message'))
   end
   if params.update_thread or params.message_thread then
      self.script = self._go:add_component('CScripted', params)
   end
end

function DynO:go()
   if self.alive then
      return self._go
   end
end

function DynO:message()
   local go = self:go()
   if not go then
      return
   end

   local msgs = go:has_message(constant.COLLIDING)
   if msgs then
      for ii, msg in ipairs(msgs) do
         local obj = DynO.find(msg.source)
         if obj then
            if msg.content == 'BEGIN' then
               if self.started_colliding_with then
                  self:started_colliding_with(obj)
               end
            elseif msg.content == 'END' then
               if self.stopped_colliding_with then
                  self:stopped_colliding_with(obj)
               end
            else
               print('unexpected message contents: ' .. msg.content)
            end
         end
      end
   end
end

function DynO:add_collider(parms)
   local go = self:go()
   if not go then
      return
   end

   if not self.colliders then
      self.colliders = {}
   end
   local c = go:add_component('CSensor', parms)
   table.insert(self.colliders, c)
   return c
end

function DynO:terminate()
   local go = self:go()
   if not go then
      return
   end

   go:delete_me(1)
   DynO.reg:unregister(go)
   self.alive = false
end

return DynO
