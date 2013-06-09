local oo = require 'oo'
local util = require 'util'
local constant = require 'constant'

local Timer = oo.class(oo.Object)

function Timer:init(owner)
   self.timer = nil
   self.owner = owner or stage
   self.msg = constant.NEXT_EPHEMERAL_MESSAGE()
   self.owner:add_component('CScripted', {message_thread=util.fthread(self:bind('evaluate'))})
end

function Timer:maybe_set(timeout, fn)
   if not self.timer then
      self.timer = self.owner:add_component('CTimer',
                                            {kind=self.msg,
                                             time_remaining=timeout})
      self.fn = fn
   end
end

function Timer:reset(timeout, fn)
   self.fn = fn
   if not self.timer then
      self.timer = self.owner:add_component('CTimer',
                                            {kind=self.msg,
                                             time_remaining=timeout})
   else
      self.timer:delete_me(0)
      self.timer:time_remaining(timeout)
   end
end

function Timer:evaluate()
   if self.owner:has_message(self.msg) then
      self.fn()
      if self.timer:delete_me() == 1 then
         self.timer = nil
         self.fn = nil
      end
   end
end

return Timer
