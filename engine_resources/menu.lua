local M = {}

local constant = require 'constant'
local util = require 'util'
local TERMINATE_MENU = constant.NEXT_EPHEMERAL_MESSAGE()

function M.make(choices, callback)
   -- we want the menu to be screen relative so tie its position to
   -- the camera
   local go = world:create_go()
   go:pos(camera:pos())

   local xpos = 100
   local ypos = 100

   local comps = {}
   local ncomps = 0
   local normal_color = {0.8, 0.8, 0.8, 1.0}
   local selected_color = {133/255.0, 190/255.0, 60/255.0, 1.0}

   local selection = 1
   for ii, choice in ipairs(choices) do
      local color = normal_color
      if ii == selection then
         color = selected_color
      end

      local comp = go:add_component('CDrawText', {offset={xpos, ypos},
                                                  atlas=constant.ATLAS,
                                                  message=choice,
                                                  color=color})

      table.insert(comps, {component=comp, choice=choice})
      ypos = ypos + 100
      ncomps = ii
   end

   local box_width = 700
   go:add_component('CTestDisplay', {offset={box_width / 2, screen_height / 2},
                                     w=box_width, h=screen_height,
                                     color={0.0, 0.0, 0.0, 0.5}})

   local updown_trigger = util.rising_edge_trigger(false)
   local time_scale = 0.01
   local player = require 'widgets/player'

   local thread = function(go, comp)
      -- disable player input so the menu has focus
      player:send_message(go:create_message(player.DISABLE_INPUT))
      world:time_scale(0)

      while true do
         coroutine.yield()
         local input = util.input_state()
         local updown = util.sign(input.updown)
         if math.abs(input.updown) < 0.1 then
            updown = 0
         end

         -- handle navigation
         if updown_trigger(updown) then
            selection = selection + updown
            if selection == 0 then
               selection = ncomps
            elseif selection > ncomps then
               selection = 1
            end
            for ii, comp in ipairs(comps) do
               local color = normal_color
               if ii == selection then
                  color = selected_color
               end
               comp.component:color(color)
            end
         end

         if input.action1 then
            world:time_scale(time_scale)
            go:add_component('CTimer', {time_remaining=0.2*time_scale, kind=TERMINATE_MENU})
            for ii, comp in ipairs(comps) do
               comp.component:delete_me(1)
            end
            return
         end
      end
   end

   local message_thread = function(go, comp)
      while true do
         coroutine.yield()
         if go:has_message(TERMINATE_MENU) then
            -- give the player input again
            world:time_scale(1)
            callback(comps[selection].choice)
            player:send_message(go:create_message(player.ENABLE_INPUT))
            go:delete_me(1)
         end
      end
   end

   go:add_component('CScripted', {update_thread=util.thread(thread),
                                  message_thread=util.thread(message_thread)})
   return go
end


return M
