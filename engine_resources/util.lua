local M = {}

local constant = require 'constant'
local rect = require 'rect'

function M.printf(...)
   print(string.format(...))
end

function M.accel(init_spd, max_height)
   return (init_spd * init_spd) / (2 * max_height)
end

function M.sign(val)
   if val > 0 then
      return 1
   elseif val < 0 then
      return -1
   else
      return 0
   end
end

function M.thread(fn)
   if not fn then
      error('thread called with null function')
   end

   local err = false
   local onerr = function(er)
      err = er
      print(debug.traceback(coroutine.running(), err, 2))
   end

   local threadfn = function(go, dt)
      local start = function()
         fn(go, dt)
      end

      xpcall(start, onerr)

      if err then
         -- an error on the following line is actually an err in fn,
         -- see the provided traceback for more detail.
         error(err)
      end
   end

   return coroutine.create(threadfn)
end

function M.fthread(inner_fn)
   local fn = function(go, comp)
      while true do
         coroutine.yield()
         inner_fn(go, comp)
      end
   end
   return M.thread(fn)
end

function M.noop_thread()
   local noop = function(go, comp)
      while true do
         coroutine.yield()
      end
   end
   return M.thread(noop)
end

function M.rand_between(lower, upper)
   local range = upper - lower
   return lower + math.random() * range
end

function M.rand_exponential(rate)
   local u = math.random()
   return math.log(1-u)/(-rate)
end

function M.rand_choice(tbl)
   local idx = math.floor(math.random() * M.count(tbl)) + 1
   return tbl[idx]
end

function M.table_copy(tbl)
   local new = {}
   for k, v in pairs(tbl) do
      new[k] = v
   end
   return new
end

function M.table_remove(tbl, obj)
   local found = nil
   for ii, v in ipairs(tbl) do
      if v == obj then
         found = ii
         break
      end
   end
   if found then
      table.remove(tbl, found)
   end
end

function M.merge_into(target, source)
   if not source then
      return target
   end

   target = M.table_copy(target)
   for k, v in pairs(source) do
      target[k] = v
   end
   return target
end

M.NOT_RUNNING = 0
M.RUNNING = 1
M.DISABLING = 2

function M.switchable_part(opts)
   local defaults = {
      on_enable = function() end,
      on_disabling = function() end,
      on_disable = function() end,
      disable_time = 0,
      initial_state = M.NOT_RUNNING
   }
   local opts = M.merge_into(defaults, opts)

   local timer_message = nil
   if opts.disable_time > 0 then
      timer_message = constant.NEXT_EPHEMERAL_MESSAGE()
   end

   local state = opts.initial_state
   local first_run = true

   local behavior_part = function(go, comp)
      if first_run and opts.initial_state == M.RUNNING then
         first_run = false
         opts.on_enable(go, comp)
      end

      if go:has_message(constant.PLAYER_ACTION) then
         if state == M.NOT_RUNNING then
            state = M.RUNNING
            opts.on_enable(go, comp)
         elseif state == M.RUNNING then
            if opts.disable_time > 0 then
               state = M.DISABLING
               go:add_component("CTimer", {time_remaining=opts.disable_time,
                                           kind=timer_message})
               opts.on_disabling(go, comp)
            else
               state = M.NOT_RUNNING
               opts.on_disabling(go, comp)
               opts.on_disable(go, comp)
            end
         end
      elseif opts.disable_time > 0 and go:has_message(timer_message) then
         state = M.NOT_RUNNING
         opts.on_disable(go, comp)
      end
   end
   return behavior_part
end

function M.switchable_particle_system_part(system, initial_state)
   local disable_time = system.max_life or 0
   local initial_state = initial_state or M.NOT_RUNNING
   local component = nil

   local switch_on = function(go, comp)
      component = go:add_component("CParticleEmitter", system)
   end

   local switch_disable = function(go, comp)
      component:active(0)
   end

   local switch_off = function(go, comp)
      component:delete_me(1)
   end

   local switchability = M.switchable_part{on_enable=switch_on,
                                           on_disabling=switch_disable,
                                           on_disable=switch_off,
                                           disable_time=disable_time,
                                           initial_state=initial_state}
   return switchability
end

function M.thread_part(part)
   local loop = function(go, comp)
      while true do
         coroutine.yield()
         part(go, comp)
      end
   end

   return M.thread(loop)
end

function M.stage_collidable(r)
   return stage:add_component("CCollidable", {fixture={type="rect",
                                                       w=rect.width(r),
                                                       h=rect.height(r),
                                                       center=rect.center(r)}})
end

function M.stage_drawrect(r)
   return stage:add_component("CTestDisplay", {w=rect.width(r),
                                               h=rect.height(r),
                                               offset=rect.center(r),
                                               color={M.rand_between(0, 1),
                                                      M.rand_between(0, 1),
                                                      M.rand_between(0, 1),
                                                      1.0}})
end

function M.rising_edge_trigger(state)
   trigger = function(input)
      local result = false
      if input ~= state and input then
         result = true
      end
      state = input
      return result
   end
   return trigger
end

function M.falling_edge_trigger(state)
   trigger = function(input)
      local result = false
      if input ~= state and (not input) then
         result = true
      end
      state = input
      return result
   end
   return trigger
end

function M.add_antigrav_force(go)
   local mass = go:mass() * go:gravity_scale()
   local grav = world:gravity()
   grav[1] = grav[1] * -mass
   grav[2] = grav[2] * -mass
   go:apply_force(grav)
end

function M.print(o)
   print(M.serialize(o))
end

function M.print_table(t)
   print('{')
   for k, v in pairs(t) do
      print(k, '=', v, ',')
   end
   print('}')
end

function M.split(str, delim, maxNb)
   -- Eliminate bad cases...
   if string.find(str, delim) == nil then
      return { str }
   end
   if maxNb == nil or maxNb < 1 then
      maxNb = 0    -- No limit
   end
   local result = {}
   local pat = "(.-)" .. delim .. "()"
   local nb = 0
   local lastPos
   for part, pos in string.gfind(str, pat) do
      nb = nb + 1
      result[nb] = part
      lastPos = pos
      if nb == maxNb then break end
   end
   -- Handle the last field
   if nb ~= maxNb then
      result[nb + 1] = string.sub(str, lastPos)
   end
   return result
end

function M.split_names(names)
   local result = {}
   for name in string.gmatch(names, '[^%s]+') do
      table.insert(result, name)
   end
   return result
end

function M.string2rgba(str)
   local result = M.split_names(str)
   return M.rgba(result[1], result[2], result[3], result[4] or 255)
end

function M.names_to_objects(names, objs)
   local result = {}
   for ii, name in ipairs(M.split_names(names)) do
      table.insert(result, objs[name])
   end
   return result
end

function M.serialize(o)
   if type(o) == "number" or type(o) == "nil" or type(o) == "boolean" then
      return tostring(o)
   elseif type(o) == "string" then
      return string.format("%q", o)
   elseif type(o) == "table" then
      local str = "{"
      for k,v in pairs(o) do
         str = str .. "  [" .. M.serialize(k) .. "] = " .. M.serialize(v) .. ','
      end
      return str .. '}'
   else
      error("cannot serialize a " .. type(o))
   end
end

function M.deserialize(str)
   if str then
      return assert(load('return ' .. str))()
   else
      return nil
   end
end

function M.set_stash(value)
   universe:stash(M.serialize(value))
end

function M.get_stash()
   return M.deserialize(universe:stash())
end

function M.add_to_stash(value)
   local stash = M.get_stash() or {}
   stash = M.merge_into(stash, value)
   M.set_stash(stash)
end

function M.set_stash_value(key, value)
   local t = {}
   t[key] = value
   M.add_to_stash(t)
end

function M.get_stash_value(key, default)
   local stash = M.get_stash()
   return (stash and stash[key]) or default
end

function M.get_level()
   return M.get_stash_value('level', 1)
end

function M.set_level(level)
   M.add_to_stash({level=level})
end

function M.increment_level()
   local new_level = M.get_level() + 1
   M.set_level(new_level)
end

function M.increment_deaths()
   local deaths = M.get_stash_value('deaths', 0)
   M.set_stash_value('deaths', deaths + 1)
end

function M.count(tbl)
   local count = 0
   for ii, v in ipairs(tbl) do
      count = ii
   end
   return count
end

function M.curry1(fn, arg1)
      local newfn = function(arg2)
         return fn(arg1, arg2)
      end
      return newfn
end

function M.contains(tbl, item)
   for k, v in pairs(tbl) do
      if v == item then
         return true
      end
   end
   return false
end

function M.loop_music(songs)
   local current_handle = nil
   local current_handle_name = M.get_stash_value('handle_name', nil)
   if current_handle_name ~= nil then
      current_handle = world:sound_handle(current_handle_name)
   end

   local play_next_music = function()
      local next_song = M.get_stash_value('song_num', 0) + 1
      if next_song > M.count(songs) then
         next_song = 1
      end
      local song = songs[next_song]

      current_handle = world:stream_sound(song, world:current_sound_sample())
      local stop_time = current_handle:last_sample()
      M.add_to_stash({song_num=next_song, song_end=stop_time,
                      handle_name=current_handle:handle()})
   end

   local keep_music_playing = function()
      local CHECK_MUSIC = constant.NEXT_EPHEMERAL_MESSAGE()
      local message_thread = function(go, comp)
         go:add_component('CTimer', {time_remaining=0, kind=CHECK_MUSIC})
         while true do
            coroutine.yield()
            if go:has_message(CHECK_MUSIC) then
               local stop_time = M.get_stash_value('song_end', 0)
               if stop_time < world:current_sound_sample() then
                  play_next_music()
               end
               go:add_component('CTimer', {time_remaining=1, kind=CHECK_MUSIC})
            end
         end
      end

      stage:add_component('CScripted', {message_thread=M.thread(message_thread)})
   end
   keep_music_playing()
end

function M.rgba(r, g, b, a)
   return {r / 255, g / 255, b / 255, a / 255}
end

function M.dedup(tbl)
   local last = nil
   local result = {}
   for ii, val in ipairs(tbl) do
      if val ~= last then
         table.insert(result, val)
      end
      last = val
   end
   return result
end

local input_state = {}

function M.button_fn(name)
   input_state[name] = false
   local fn = function(value)
      if value == 0 then
         input_state[name] = false
      else
         input_state[name] = true
      end
   end
   return fn
end

function M.axis_fn(name)
   input_state[name] = 0
   local fn = function(value)
      input_state[name] = value
   end
   return fn
end

function M.install_keymap(mapping)
   for name, fn in pairs(mapping) do
      assert(world:set_keybinding(name, fn))
   end
end

function M.install_basic_keymap()
   local keymap = {
      z = M.button_fn('action1'),
      x = M.button_fn('action2'),
      c = M.button_fn('action3'),
      a = M.button_fn('action4'),
      K_LEFTRIGHT = M.axis_fn('leftright'),
      K_UPDOWN = M.axis_fn('updown'),
      K_JOY1 = M.button_fn('action1'),
      K_JOY2 = M.button_fn('action2'),
      K_JOY3 = M.button_fn('action3'),
      K_JOY4 = M.button_fn('action4')
   }

   M.install_keymap(keymap)
end

function M.input_state()
   return input_state
end

function M.d2r(deg)
   return math.pi * deg / 180.0
end

local vector = require 'vector'

function M.forward_direction(go)
   local angle = go:angle()
   return vector.new({-math.sin(angle), math.cos(angle)})
end

function M.lerp(a, b, s)
   local d = b - a
   return a + (s * d)
end

function M.interpolate(vals, val)
   local last_val = nil
   for ii, pair in ipairs(vals) do
      if not last_val then
         -- skip
      else
         if val >= last_val[1] and val <= pair[1] then
            local s = (val - last_val[1]) / (pair[1] - last_val[1])
            return M.lerp(last_val[2], pair[2], s)
         end
      end
      last_val = pair
   end

   if val > last_val[1] then
      return last_val[2]
   elseif val < vals[1][1] then
      return vals[1][2]
   else
      print('interpolate is confused')
      return 0 -- hack
   end
end

return M
