local M = {}

local enter_state = function(fsm, state, last_state)
   --print('entering state ' .. state .. ' from ' .. last_state)

   fsm.last_state = last_state
   fsm.state = state
   fsm.state_time = 0

   if fsm.states[fsm.state].enter then
      fsm.states[fsm.state].enter(fsm)
   end
end

function M.new(fsm)
   setmetatable(fsm, M)
   return fsm
end

function M.step(fsm)
   -- if not state then go to start state
   if not fsm.state then
      enter_state(fsm, fsm.start_state, nil)
   end

   fsm.state_time = fsm.state_time + world:dt()

   -- perform update
   local current_state = fsm.state
   fsm.states[fsm.state].update(fsm)
   if current_state ~= fsm.state then
      enter_state(fsm, fsm.state, current_state)
   end
end

M.__index = M

return M
