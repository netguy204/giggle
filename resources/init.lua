package.path = package.path .. ';resources/?.lua'

local util = require 'util'
local vector = require 'vector'
local constant = require 'constant'
local map = require 'map'
local rect = require 'rect'
local oo = require 'oo'

-- set by init
local level_data = nil
local level_number = 1

local kills_remaining = 0
local sounds = {}

local ATLAS = 'resources/default'

function play_sound(name)
   local opts = sounds[name]
   if not opts then
      return
   else
      local snd = util.rand_choice(opts)
      world:play_sound(snd, constant.EVENT)
   end
end

local Registry = oo.class(oo.Object)
function Registry:init()
   self.db = {}
end

function Registry:register(go, obj)
   self.db[go:key()] = obj
end

function Registry:unregister(go)
   self.db[go:key()] = nil
end

function Registry:find(go)
   return self.db[go:key()]
end

-- global registry
local reg = Registry()

local GO = oo.class(oo.Object)
function GO:init(go, w, h)
   reg:register(go, self)
   self.go = go
   self.w = w
   self.h = h
end

function GO:destroy()
   reg:unregister(self.go)
   self.go:delete_me(1)
end

function GO:rect()
   local pos = self.go:pos()
   local hw = self.w / 2
   local hh = self.h / 2
   return {pos[1] - hw, pos[2] - hh, pos[1] + hw, pos[2] + hh}
end

function GO:test_collision(other)
   return rect.intersect(self:rect(), other:rect())
end

local enemies = {}

local Enemy = oo.class(GO)

function Enemy:init(go, dir, w, h)
   GO.init(self, go, w, h)
   table.insert(enemies, self)

   self.dir = dir
   self.speed = level_data.speed
   self._anim = world:animation('resources/enemy.cs', ATLAS, 'NewAnimation')
   self.speed_scales = {{0,0.5}, {0.279, 0}, {0.230, 1.0}, {0.550, 1.5}, {0.551, 0.5}, {0.840, 0}}

   self:setup_go()
end

function Enemy:destroy()
   GO.destroy(self)

   -- remove from list (by producing new list)
   local newlst = {}
   for ii, other in ipairs(enemies) do
      if other ~= self then
         table.insert(newlst, other)
      end
   end

   enemies = newlst
end

function Enemy:setup_go()
   if self.dir > 0 then
      self.go:pos({-self.w/4, 128})
   else
      self.go:pos({screen_width + self.w/4, 128})
   end

   self.go:body_type(constant.DYNAMIC)
   self.anim = self.go:add_component('CSpriterSprite',
                                     {animation=self._anim,
                                      offset={-2,-12},
                                      time_scale=self.speed/100,
                                      scale_x=self.dir,
                                      scale_y=1,
                                      layer=constant.PLAYER})

   --self.go:add_component('CTestDisplay', {w=self.w, h=self.h})
   self.go:add_component('CPlatformer', {w=self.w, h=self.h})
end

function Enemy:should_terminate()
   local pos = self.go:pos()
   return (self.dir > 0 and pos[1] > (screen_width + self.w / 2))
      or (self.dir < 0 and pos[1] < -(self.w / 2))
end

function Enemy:update()
   local time = self.anim:current_time() % self._anim.length
   local speed_scale = util.interpolate(self.speed_scales, time)
   if speed_scale < 0 then
      print('baddness', speed_scale, time)
   end
   local dv = (speed_scale * self.speed * self.dir) - self.go:vel()[1]
   self.go:apply_impulse({dv * self.go:mass(), 0})
end

-- capture mouse positioning
local mouse_pos = {0,0}
local mouse_callback = function(x, y, rx, ry)
   mouse_pos = {x, y}
end
world:set_sibinding('SI_MOUSE', mouse_callback)
-- end mouse

local MouseEnemy = oo.class(Enemy)

function MouseEnemy:update()
   self.go:pos(mouse_pos)
end

-- a square that jumps on circles but avoids squares

function spawn_enemy()
   local go = world:create_go()
   local dir = 1
   if random_gaussian() < 0 then
      dir = -1
   end

   return Enemy(go, dir, 48, 24)
end

function update_enemies(player)
   for ii, enemy in ipairs(enemies) do
      enemy:update()

      if enemy:should_terminate() then
         enemy:destroy()
      end
   end
end

local Player = oo.class(GO)
function Player:init(go, w, h)
   GO.init(self, go, w, h)
   self.health = 1
   self.armor_factor = 1
end

function Player:take_damage(damage_rate)
   local value = damage_rate * world:dt()
   self.health = math.max(0, self.health - (self.armor_factor * value))
end

function Player:dead()
   return self.health == 0
end

local Bar = oo.class(GO)
function Bar:init(go, w, h)
   GO.init(self, go, w, h)
   self.v = 1
   local offset = {w/2, 0}
   self.back = go:add_component('CTestDisplay',
                                {w=w, h=h, offset=offset,
                                 color={0.5, 0.5, 0.5, 1.0},
                                 layer=constant.PLAYER})
   self.fore = go:add_component('CTestDisplay',
                                {w=w, h=h, offset=offset,
                                 color={0.0, 1.0, 0.0, 1.0},
                                 layer=constant.FOREGROUND})
end

function Bar:value(value)
   if not value then
      return self.v
   else
      local w = self.w * value
      local offset = {self.w/2, 0}
      self.fore:w(w)
      self.fore:offset(offset)
      self.v = value
   end
end

local Timer = oo.class(oo.Object)
Timer.timers = {}

function Timer.register()
   local thread = function(go, comp)
      while true do
         coroutine.yield()
         for ii, timer in ipairs(Timer.timers) do
            timer:evaluate()
         end
      end
   end
   stage:add_component('CScripted', {message_thread=util.thread(thread)})
end

function Timer:init()
   self.timer = nil
   self.msg = constant.NEXT_EPHEMERAL_MESSAGE()
   table.insert(Timer.timers, self)
end

function Timer:maybe_set(timeout, fn)
   if not self.timer then
      self.timer = stage:add_component('CTimer',
                                       {kind=self.msg,
                                        time_remaining=timeout})
      self.fn = fn
   end
end

function Timer:reset(timeout, fn)
   self.fn = fn
   if not self.timer then
      self.timer = stage:add_component('CTimer',
                                       {kind=self.msg,
                                        time_remaining=timeout})
   else
      self.timer:time_remaining(timeout)
   end
end

function Timer:evaluate()
   if stage:has_message(self.msg) then
      self.fn()
      self.timer = nil
      self.fn = nil
   end
end

local Camera = oo.class(GO)
function Camera:init(go, w, h)
   GO.init(self, go, w, h)
   self.k = 1000
   self.d = 10
   self.m = 1
   self.target = vector.new({0,0})
   self.cam = stage:find_component("Camera", nil)

   go:body_type(constant.DYNAMIC)
end

function Camera:update()
   local dt = world:dt()
   local pos = vector.new(self.go:pos())
   local vel = vector.new(self.go:vel())

   util.add_antigrav_force(self.go)

   if vel:length() < 100 then
      self.go:vel({0,0})
      self.go:pos(self.target)
   else
      local offset = self.target - pos

      local spring_force = offset:length() * self.k
      local damp_force = vel:length() * self.d

      local force = vector.new({0,0})
      if offset:length() > 0.001 then
         force = force + (offset:norm() * spring_force)
      end
      if vel:length() > 0.001 then
         force = force + (vel:norm() * -damp_force)
      end

      self.go:apply_force(force)
   end
end

function Camera:set_ortho(minx, maxx, miny, maxy)
   self.cam:world2camera():orthographic_proj(minx, maxx, miny, maxy, -1, 1)
end

function Camera:set_viewport()
   local pos = self.go:pos()
   self:set_ortho(pos[1], pos[1] + self.w, pos[2], pos[2] + self.h)
end

function reset_state(new_state)
   util.add_to_stash({state=new_state, level=level_number})
   local reset = function()
      reset_world()
   end
   Timer():reset(0, reset)
end

function make_particle_system(player)
   local _smoke = world:atlas_entry(ATLAS, 'steam')

   return player:add_component('CParticleSystem',
                               {def=
                                   {n=1000,
                                    renderer={name='PSC_E2SystemRenderer',
                                              params={entry=_smoke}},
                                    activator={name='PSConstantRateActivator',
                                               rate=0},
                                    components={
                                       {name='PSConstantAccelerationUpdater',
                                        params={acc={0,10}}},
                                       {name='PSTimeAlphaUpdater',
                                        params={time_constant=1.5}},
                                       {name='PSBoxInitializer',
                                        params={initial={-32,-32,32,-32},
                                                refresh={-32,-32,32,-32},
                                                minv={-1000,-10},
                                                maxv={1000,10}}},
                                       {name='PSConstColorInitializer',
                                        params={color=util.rgba(160, 160, 160, 255)}},
                                       {name='PSTimeInitializer',
                                        params={min_life=0.2,
                                                max_life=3.0}},
                                       {name='PSTimeTerminator'}}}})
end

local camera = Camera(world:create_go(), screen_width, screen_height)

function game()
   --debugging collisions
   --MouseEnemy(world:create_go(), 1, 32, 32)
   local flash_red = {0.8, 0.0, 0.0, 0.6}
   local flash_off = {0.0, 0.0, 0.0, 0.0}

   kills_remaining = level_data.goal

   local flash = stage:add_component('CTestDisplay',
                                     {w=screen_width, h=screen_height,
                                      offset={screen_width/2, screen_height/2},
                                      color=flash_off,
                                      layer=constant.MENU})

   local score = stage:add_component('CDrawText',
                                     {offset={screen_width/2 - 32, screen_height - 128},
                                     atlas=ATLAS,
                                     message=string.format("%d", kills_remaining),
                                     color={1.0,1.0,1.0,1.0}})

   local gobar = world:create_go()
   gobar:pos({16, screen_height-32})
   local bar = Bar(gobar, screen_width-32, 16)

   local player = world:create_go()
   local _psystem = make_particle_system(player)
   local psys = _psystem:def():find_component('PSConstantRateActivator')

   local pdim = 64
   local pobj = Player(player, pdim, pdim)
   bar:value(pobj.health)

   player:pos({screen_width/2, 128})
   local jump_speed = 800
   --player:add_component('CTestDisplay', {w=pdim, h=pdim,
   --                                      color={0.9, 0.9, 0.9, 1.0}})
   local pformer = player:add_component('CPlatformer', {w=pdim, h=pdim})

   local _idle_anim = world:animation('resources/PlayerIdle.cs', ATLAS, 'Idle')
   local _jump_anim = world:animation('resources/PlayerJump.cs', ATLAS, 'NewAnimation')

   local anim = player:add_component('CSpriterSprite',
                                     {animation=_idle_anim,
                                      layer=constant.PLAYER,
                                      offset={0,-2}})

   local sh = 32
   local sw = 4

   local s1_center = {-(pdim/2),-(pdim/2-sh/2)}
   local s2_center = {(pdim/2),-(pdim/2-sh/2)}
   player:add_component('CSensor',
                        {fixture={type='rect',
                                  sensor=true,
                                  w=sw, h=sh,
                                  center=s1_center}})
   --player:add_component('CTestDisplay',
   --                     {w=sw, h=sh,
   --                      offset=s1_center})

   player:add_component('CSensor',
                        {fixture={type='rect',
                                  sensor=true,
                                  w=sw, h=sh,
                                  center=s2_center}})
   --player:add_component('CTestDisplay',
   --                     {w=sw, h=sh,
   --                      offset=s2_center})

   local flash_timer = Timer()
   local smoke_timer = Timer()

   local pthread = function(go, comp)
      local jumping = false
      while true do
         coroutine.yield()
         camera:update()
         camera:set_viewport()

         local input = util.input_state()

         if pformer:parent() and input.action1 then
            player:apply_impulse({0, jump_speed})
            anim:animation(_jump_anim)
            anim:current_time(0)
            jumping = true
         elseif pformer:parent() and (not input.action1) then
            anim:animation(_idle_anim)
         elseif jumping and not input.action1 then
            local vel = go:vel()
            if vel[2] > 0 then
               go:apply_impulse({0, go:mass() * -vel[2]})
               jumping = false
            end
         end

         if pobj:dead() or player:pos()[2] < 0 then
            reset_state('game_lose')
         end
      end
   end

   local pmthread = function(go, comp)
      while true do
         coroutine.yield()

         local msg = go:has_message(constant.PARENT_CHANGE)
         if msg then
            local parent = pformer:parent()
            if parent then
               camera.go:apply_impulse({0,1000})

               local other = parent and reg:find(pformer:parent())

               if other and other:is_a(Enemy) then
                  other:destroy()
                  psys:rate(10000)
                  local disable = function()
                     psys:rate(0)
                  end
                  smoke_timer:reset(0, disable)

                  -- check for win state
                  kills_remaining = kills_remaining - 1
                  play_sound('hits')
                  score:message(string.format('%d', kills_remaining))
                  if kills_remaining <= 1 then
                     reset_state('game_win')
                  end
               else
                  play_sound('floors')
               end
            end
         else
            -- don't look for side collisions if we're on top of an enemy
            msg = go:has_message(constant.COLLIDING)
            if msg then
               local other = reg:find(msg.source)
               if other and other:is_a(Enemy) then
                  pobj:take_damage(0.25)
                  bar:value(pobj.health)
                  flash:color(flash_red)
                  local disable = function()
                     flash:color(flash_off)
                  end
                  flash_timer:maybe_set(0.1, disable)
               end
            end
         end
      end
   end
   player:add_component('CScripted', {update_thread=util.thread(pthread),
                                      message_thread=util.thread(pmthread)})

   local spawn_period = 1
   local SPAWN_ENEMY = constant.NEXT_EPHEMERAL_MESSAGE()
   local timer = stage:add_component('CTimer', {kind=SPAWN_ENEMY,
                                                time_remaining=spawn_period})
   local ethread = function(go, comp)
      while true do
         coroutine.yield()
         if go:has_message(SPAWN_ENEMY) then
            timer:delete_me(0)
            timer:time_remaining(util.rand_exponential(spawn_period))
            spawn_enemy()
         end
      end
   end
   local euthread = function(go, comp)
      while true do
         coroutine.yield()
         update_enemies(pobj)
      end
   end
   stage:add_component('CScripted', {message_thread=util.thread(ethread),
                                     update_thread=util.thread(euthread)})

   -- add a surface
   local sr = {0, 0, screen_width, 64}
   util.stage_collidable(sr)
   util.stage_drawrect(sr)
end

function menu_screen(message, offset, fn)
   local title = stage:add_component('CDrawText', {offset={screen_width/2 - 128, screen_height * 2.0 / 3.0},
                                                   atlas=ATLAS,
                                                   message="Press Z",
                                                   color={1.0,0.0,0.0,1.0}})

   stage:add_component('CDrawText', {offset=offset,
                                     atlas=ATLAS,
                                     message=message,
                                     color={1.0,1.0,1.0,1.0}})

   local falling_edge = util.falling_edge_trigger(false)

   local red_max = 1.0
   local red_min = 0.3
   local red_level = red_min
   local red_dir = 1
   local thread = function(go, comp)
      while true do
         coroutine.yield()
         if falling_edge(util.input_state().action1) then
            fn()
         end
         red_level = red_level + red_dir * world:dt()
         if red_level > red_max then
            red_level = red_max
            red_dir = -red_dir
         elseif red_level < red_min then
            red_level = red_min
            red_dir = -red_dir
         end
         title:color({red_level, 0.0, 0.0, 1.0})
      end
   end
   local install_handler = function()
      stage:add_component('CScripted', {update_thread=util.thread(thread)})
   end
   Timer():reset(0.2, install_handler)
end

function game_lose()
   play_sound('loses')
   local go_to_init = function()
      reset_state('game_init')
   end
   menu_screen(string.format('LEVEL FAILED'),
               {screen_width/2-200, screen_height/2},
               go_to_init)
   level_number = 1
end

function game_win()
   play_sound('wins')
   local go_to_init = function()
      reset_state('game_init')
   end
   menu_screen(string.format('LEVEL CLEARED'),
               {screen_width/2-220, screen_height/2},
               go_to_init)
   level_number = level_number + 1
end

function game_init()
   local go_to_game = function()
      reset_state('game')
   end
   menu_screen(string.format("STOMP %d",
                             level_data.goal),
               {screen_width/2-150, screen_height/2},
               go_to_game)

   stage:add_component('CDrawText', {offset={screen_width/2 - 128, screen_height / 3.0},
                                     atlas=ATLAS,
                                     message=string.format('Level %d', level_number),
                                     color={0.6,0.6,0.6,1.0}})

end

function enable_compositor()

   local tex = nil
   local fbo = nil
   local czor = world:create_object('Compositor')
   local tform = czor:transform_create()
   tform:orthographic_proj(0, screen_width, 0, screen_height, -1, 1)

   local pre_render = function(go, comp)
      while true do
         coroutine.yield()
         if not tex then
            tex = czor:texture_create(screen_width/2, screen_height/2, constant.GL_NEAREST)
            fbo = czor:frame_buffer_create(tex)
         end
         czor:frame_buffer_bind(fbo)
         czor:clear_with_color({0.2,0.2,0.2,1.0})
      end
   end

   local post_render = function(go, comp)
      while true do
         coroutine.yield()
         czor:frame_buffer_bind(nil)
      end
   end

   local world_post_render = function()
      while true do
         coroutine.yield()
         czor:transform_set(tform)
         czor:clear_with_color({0, 0, 0, 1})
         local w2 = screen_width/2
         local h2 = screen_height/2
         czor:textured_quad({0,0, w2, h2}, {1,0,0,1}, tex)
         czor:textured_quad({w2,0,screen_width,h2}, {0,1,0,1}, tex)
         czor:textured_quad({w2,h2,screen_width,screen_height}, {0,0,1,1}, tex)
         czor:textured_quad({0,h2,w2,screen_height}, {1,1,1,1}, tex)
      end
   end
   stage:find_component('Camera', nil):pre_render(util.thread(pre_render))
   stage:find_component('Camera', nil):post_render(util.thread(post_render))
   world:post_render(util.thread(world_post_render))
end

function level_init()
   Timer.register()
   util.install_basic_keymap()
   enable_compositor()

   sounds={
      hits={world:get_sound('sounds/hit1.ogg', 1),
            world:get_sound('sounds/hit2.ogg', 1),
            world:get_sound('sounds/hit3.ogg', 1),
            world:get_sound('sounds/hit4.ogg', 1),
            world:get_sound('sounds/hit5.ogg', 1),
            world:get_sound('sounds/hit6.ogg', 1)},
      jumps={world:get_sound('sounds/jump1.ogg', 1)},
      wins={world:get_sound('sounds/win1.ogg', 1)},
      loses={world:get_sound('sounds/lose1.ogg', 1)},
      floors={world:get_sound('sounds/floor1.ogg', 1)}}

   local levels = {{speed=100, rate=1, goal=10},
                   {speed=100, rate=2, goal=10},
                   {speed=200, rate=1, goal=20},
                   {speed=200, rate=2, goal=30}}
   local nlevels = util.count(levels)

   local stash = util.get_stash() or {state='game_init', level=1}

   local state = stash.state
   level_number = stash.level

   if level_number <= nlevels then
      level_data = levels[level_number]
   else
      local last = levels[nlevels]
      local scale = 1 + level_number - nlevels
      level_data = {speed=last.speed * scale,
                    rate=last.rate * scale,
                    goal=last.goal * scale}
   end

   local states = {game = game,
                   game_init = game_init,
                   game_win = game_win,
                   game_lose = game_lose}
   states[state]()
end
