local util = require 'util'

local M = {};

local sfx = {}

function M.load(kind, names)
   sfx[kind] = {}
   for ii, name in ipairs(names) do
      table.insert(sfx[kind], game:get_sound(name, 1.0))
   end
end

function M.play(kind)
   local snd = util.rand_choice(sfx[kind])
   game:play_sound(snd, 1)
end

return M
