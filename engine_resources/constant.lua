local M = {}

M.ATLAS = "resources/default"
M.TILES = "resources/tiles"
M.BIG = "resources/big"

-- messages
M.COLLIDING = 0
M.TIMER_EXPIRED = 1
M.EXPLOSION_NEARBY = 2
M.PARENT_CHANGE = 3
M.BRAIN_COMPLETE = 4

-- script-only messages
M.PLAYER_ACTION = 100
M.PLAYER_JUMP = 101
M.PLAYER_JUMP_ABORT = 102

local function __make_ephemeral_counter()
   next_message = 1000
   local fn = function()
      local message = next_message
      next_message = next_message + 1
      return message
   end
   return fn
end

M.NEXT_EPHEMERAL_MESSAGE = __make_ephemeral_counter()
M.ACTIVATE_SWITCHED = M.NEXT_EPHEMERAL_MESSAGE()
M.ALT_ACTIVATE_SWITCHED = M.NEXT_EPHEMERAL_MESSAGE()
M.ACTIVATE = M.NEXT_EPHEMERAL_MESSAGE()

-- layers

M.BACKERDROP = 0
M.BACKDROP = 1
M.BACKERGROUND = 2
M.BACKGROUND = 3
M.PLAYER = 4
M.FOREGROUND = 5
M.MENU = 6

-- colorings
M.BLACKBODY = 0
M.BW = 1

-- channels
M.EVENT = 0
M.FOLEY = 1
M.AMBIANCE = 2
M.STREAM = 3

-- body types
M.STATIC = 0
M.KINEMATIC = 1
M.DYNAMIC = 2

-- opengl
M.GL_NEAREST = 0x2600
M.GL_LINEAR = 0x2601

-- tile bitmasks, keep in sync with tiles.h
M.COLLIDABLE = 1
M.VISIBLE = 2
M.PASSABLE = 3

-- raycast types, gameobject.h
M.CLOSEST = 0
M.EVERYTHING = 1
M.RANGE = 2

-- mesh types, mesh.h
M.LINES = 0
M.TRIS = 1

return M
