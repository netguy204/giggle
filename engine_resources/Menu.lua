local oo = require 'oo'
local util = require 'util'
local Indicator = require 'Indicator'

local Menu = oo.class(oo.Object)
function Menu:init(font, options)
   local screen_rect = Rect(camera:viewport())
   local screen_width = screen_rect:width()
   local screen_height = screen_rect:height()

   local world = game:create_world()
   local go = world:create_go()
   local indicators = {}

   local y = screen_height / 2 + font:line_height() * #options / 2
   local x = screen_width / 2
   for ii = 1,#options do
      local bg = Indicator(font, {x-1, y-1}, {0,0,0,1}, go)
      local choice = Indicator(font, {x, y}, {0,0,0,0}, go)
      choice:update(options[ii][1])
      bg:update(options[ii][1])
      table.insert(indicators, choice)
      y = y - font:line_height()
   end

   self.world = world
   self.go = go
   self.indicators = indicators
   self.state = 1
   self.options = options
   self.up = util.rising_edge_trigger(false)
   self.down = util.rising_edge_trigger(false)
   self.select = util.falling_edge_trigger(false)
   go:add_component('CScripted', {update_thread=util.fthread(self:bind('update'))})
end

function Menu:update()
   local input = util.input_state()
   local options = self.options
   local state = self.state
   local fire = false

   if self.up(input.updown > 0.1 or input.leftright < -0.1) then
      state = state - 1
      if state <= 0 then
         state = #options
      end
   elseif self.down(input.updown < -0.1 or input.leftright > 0.1) then
      state = state + 1
      if state > #options then
         state = 1
      end
   elseif self.select(input.action2) then
      fire = true
   end

   for ii = 1, #options do
      if ii == state then
         self.indicators[ii]:color({1,1,1,1})
      else
         local c = .4
         self.indicators[ii]:color({c,c,c,1})
      end
   end

   if fire then
      local option = self.options[state]
      option[2]()
   end

   self.state = state
end

function Menu:terminate()
   self.world:delete_me(1)
end

return Menu
