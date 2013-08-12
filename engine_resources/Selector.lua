local oo = require 'oo'
local rect = require 'rect'
local FSM = require 'FSM'

local Selector = oo.class(oo.Object)

function Selector:init(opts)
   opts = opts or {}
   self.color = opts.color or {0.7, 0.7, 1.0, 0.5}

   local states = {
      start = {
         enter = function()
            self.current_selection = nil
         end,

         update = function(fsm, input, mouse)
            if input.mouse1 then
               self.select_start = mouse
               if opts.start_callback then
                  opts.start_callback(mouse)
               end
               fsm:enter('selecting')
            end
         end
      },

      selecting = {
         update = function(fsm, input, mouse)
            self.current_selection = rect.corners(self.select_start, mouse)

            if not input.mouse1 then
               if opts.select_callback then
                  opts.select_callback(self.current_selection)
               end
               self.last_selection = self.current_selection
               fsm:enter('start')
            end
         end
      }
   }

   self.fsm = FSM(states)
end

function Selector:update(input, mouse)
   self.fsm:update(input, mouse)
   if self.current_selection then
      if not self.visual then
         self.visual = stage:add_component('CTestDisplay', {color=self.color})
      end

      self.visual:offset(rect.center(self.current_selection))
      self.visual:w(rect.width(self.current_selection))
      self.visual:h(rect.height(self.current_selection))
   elseif self.visual then
      self.visual:delete_me(1)
      self.visual = nil
   end
end

return Selector
