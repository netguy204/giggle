local M = {}
local oo = require 'oo'

local FSM = oo.class(oo.Object)

function FSM:init(states)
   self.states = states
end

function FSM:enter(state)
   --print('entering state ' .. state .. ' from ' .. last_state)

   self.last_state = self.state
   self.state = state

   if self.states[self.state].enter then
      self.states[self.state].enter(self)
   end
end

function FSM:update()
   -- if not state then go to start state
   if not self.state then
      self:enter('start')
   end

   -- perform update
   self.states[self.state].update(self)
end

return FSM
