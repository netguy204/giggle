local M = {}

local constant = require 'constant'
local util = require 'util'

M.defaults = ""

function M.add_clouds(self, _art)
   local clouds =
      stage:add_component('CParticleSystem',
                          {def=
                           {n=4,
                            layer=constant.BACKDROP,
                            renderer={name='P_ES2SystemRenderer',
                                      params={entry=_art,
                                              scale=1.4}},
                            components={
                               {name='PSConstantAccelerationUpdater',
                                params={acc={0, 0}}},
                               {name='PSBoxInitializer',
                                params={initial={-_art.w, -_art.h,
                                                 screen_width + _art.w,
                                                 screen_height + _art.h},
                                        refresh={-_art.w, -_art.h,
                                                 -_art.w,
                                                 screen_height + _art.h},
                                        minv={5, -5},
                                        maxv={10, 5}}},
                               {name='PSBoxTerminator',
                                params={rect={-_art.w*2, -_art.h*2,
                                              screen_width + _art.w * 2,
                                              screen_height + _art.h * 2}}}}}})
      table.insert(self.systems, clouds)
      return clouds
end

function M.new()
   local obj = {}
   setmetatable(obj, M)
   obj.systems = {}
   return obj
end

function M.clouds(self)
   local _art1 = world:atlas_entry(constant.ATLAS, 'cloud-bit-1')
   local _art2 = world:atlas_entry(constant.ATLAS, 'cloud-bit-2')
   local _art3 = world:atlas_entry(constant.ATLAS, '/xcloud-bit-1')
   local _art4 = world:atlas_entry(constant.ATLAS, '/xcloud-bit-2')
   self:add_clouds(_art1)
   self:add_clouds(_art2)
   self:add_clouds(_art3)
   self:add_clouds(_art4)
end

function M.stars(self)
   local _art = world:atlas_entry(constant.ATLAS, 'star')
   local stars =
      stage:add_component('CParticleSystem',
                          {def=
                           {n=200,
                            layer=constant.BACKDROP,
                            renderer={name='PSC_E2SystemRenderer',
                                      params={entry=_art}},
                            components={
                               {name='PSConstantAccelerationUpdater',
                                params={acc={0, 0}}},
                               {name='PSBoxInitializer',
                                params={initial={-_art.w, -_art.h,
                                                 screen_width + _art.w,
                                                 screen_height + _art.h},
                                        refresh={-_art.w, -_art.h,
                                                 -_art.w,
                                                 screen_height + _art.h},
                                        minv={0, 0},
                                        maxv={5, 0}}},
                               {name='PSRandColorInitializer',
                                params={min_color={0.8, 0.8, 0.8, 0.2},
                                        max_color={1.0, 1.0, 1.0, 1.0}}},
                               {name='PSRandScaleInitializer',
                                params={min_scale=0.2,
                                        max_scale=0.6}},
                               {name='PSBoxTerminator',
                                params={rect={-_art.w*2, -_art.h*2,
                                              screen_width + _art.w * 2,
                                              screen_height + _art.h * 2}}}}}})
      table.insert(self.systems, stars)
end

function M.sun(self, map)
   local default_h = tostring(screen_height - screen_height / 4)
   local height = tonumber(map.properties.sun_height or default_h)
   M:add_sun(height)
end

function M.set_gradient(obj, top_c, bottom_c)
   obj.rect:bl_c(bottom_c)
   obj.rect:br_c(bottom_c)
   obj.rect:tl_c(top_c)
   obj.rect:tr_c(top_c)
end

M.__index = M

function M.add_sun(self, sun_y)
   local _sun = world:atlas_entry(constant.BIG, 'sun')
   local sun_x = util.rand_between(screen_width - screen_width / 3, screen_width)
   local sun = stage:add_component('CStaticSprite', {entry=_sun, offset={sun_x, sun_y},
                                                     layer=constant.BACKERDROP})
   local rotate_sun = function(go, comp)
      local rotation_speed = 0.03
      while true do
         coroutine.yield()
         local angle = sun:angle() + rotation_speed * world:dt()
         sun:angle(angle)
      end
   end
   stage:add_component('CScripted', {update_thread=util.thread(rotate_sun)})
end

return M
