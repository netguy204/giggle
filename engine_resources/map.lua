local M = {}
local util = require 'util'
local Rect = require 'Rect'
local constant = require 'constant'

-- flips the map so that the top row ends up on the top of the screen
-- like you would expect if you just visualized the map numbers
local function flip_map(map)
   local new_tiles = {}
   local ntiles = map.width * map.height
   for idx=1,ntiles do
      row = math.floor((idx - 1) / map.width) + 1
      col = (idx - 1) % map.width + 1
      new_idx = (map.height - row) * map.width + col
      new_tiles[idx] = map.tiles[new_idx]
   end
   local new_map = util.table_copy(map)
   new_map.tiles = new_tiles
   return new_map
end

function M.create(map)
   map = flip_map(map)
   map.__index = M
   setmetatable(map, map)
   return map
end

local function slurp_lines(filename)
   local f = assert(io.open(filename))
   local result = {}
   for line in f:lines() do
      table.insert(result, line)
   end
   io.close(f)
   return result
end

function M.from_file(map_file, atlas, order_file)
   local map = require(map_file)
   local order = slurp_lines(order_file)

   -- build the specs
   local specs = {}
   local metadata = {}
   for ts_idx, ts in ipairs(map.tilesets) do
      for i, entry in ipairs(ts.tiles) do
         metadata[entry.id + 1] = entry
      end
   end

   for i, name in ipairs(order) do
      local art = world:atlas_entry(atlas, name)
      local spec = {image=art}

      local entry = metadata[i]
      if entry and entry.properties then
         for pname, pbool in pairs(entry.properties) do
            spec[pname] = (pbool == "true")
         end
      end
      table.insert(specs, spec)
   end

   local m = {
      width = map.width,
      height = map.height,
      tile_width = map.tilewidth,
      tile_height = map.tileheight,
      specs = specs,
      properties = map.properties
   }

   -- we only support one layer of map data. make sure that's all we
   -- get
   local found_tilelayer = false
   local built_map = nil
   for i, layer in ipairs(map.layers) do
      if layer.type == "tilelayer" then
         if found_tilelayer then
            error('found more than 1 tilelayer')
         else
            found_tilelayer = true
         end

         m['tiles'] = layer.data
         built_map = M.create(m)
      elseif layer.type == 'objectgroup' then
         -- object layer, skip for now
         local mw = m.width * m.tile_width
         local mh = m.height * m.tile_height
         local objs = {player=player,
                       stage=stage}
         for i, obj in ipairs(layer.objects) do
            local go = world:create_go()
            go:pos(built_map:tiled2map_pos({obj.x, obj.y}))
            objs[obj.name] = go
         end

         for i, obj in ipairs(layer.objects) do
            local go = objs[obj.name]
            local mod = require('widgets/'..obj.type)
            mod.cons(go, built_map, obj, objs)
         end
      end
   end

   return built_map
end

function M.tiled2map_pos(m, pos)
   return { pos[1] + m.tile_width / 2,
            ((m.height * m.tile_height) - pos[2]) - m.tile_height / 2 }
end

function M.tiled2polyline(m, offset, polyline)
   local result = {}
   for ii, point in ipairs(polyline) do
      local p = offset + {point.x, -point.y}
      if (not result[1]) or (p - result[1]):length() > 0.001 then
         table.insert(result, p)
      end
   end
   return result
end

function M.index(map, row, col)
   if row < 1 or row > map.height or col < 1 or col > map.width then
      return nil
   end
   return (row - 1) * map.width + col
end

function M.fetch(map, row, col)
   local idx = map:index(row, col)
   if not idx then
      return {}
   end

   local speci = map.tiles[idx]
   if speci == 0 then
      return {index=idx, row=row, col=col}
   else
      local new_table = util.table_copy(map.specs[speci])
      new_table.index = idx
      new_table.row = row
      new_table.col = col
      return new_table
   end
end

function M.center(map, row, col)
   local x_bl = map.tile_width * (col - 1)
   local y_bl = map.tile_height * (row - 1)
   return {x_bl + map.tile_width / 2,
           y_bl + map.tile_height / 2}
end

function M.query(map, point)
   local col = math.floor(point[1] / map.tile_width) + 1
   local row = math.floor(point[2] / map.tile_height) + 1
   return map:fetch(row, col)
end

function M.query_rect_corners(map, r, kind)
   local v = map:query_rect_top(r, kind)
   if v then
      return v
   end
   return map:query_rect_bottom(r, kind)
end

function M.query_rect_top(map, r, kind)
   local v = map:query(r:bl())
   v = map:query(r:tl())
   if v[kind] then
      return v
   end

   v = map:query(r:tr())
   if v[kind] then
      return v
   end
   return false
end

function M.query_rect_bottom(map, r, kind)
   local v = map:query(r:bl())
   if v[kind] then
      return v
   end

   v = map:query(r:br())
   if v[kind] then
      return v
   end
   return false
end



-- install the map into the stage
function M.update_visuals(map)
   if map.draw_component then
      map.draw_component:map(map)
   else
      -- hack. the map caches and doesn't know it needs to refresh
      -- when we manipulate the transform
      local mw = map.width * map.tile_width
      local mh = map.height * map.tile_height
      local params = {map=map, w=mw, h=mh,
                      layer=constant.BACKGROUND}
      map.draw_component = stage:add_component("CDrawTilemap", params)
   end
end

function M.update_colliders(map)
   -- if we already had colliders, remove them
   if map.colliders then
      for k, v in pairs(map.colliders) do
         v:delete_me(1)
      end
   end

   -- trying to build a fairly minimal set of rectangular colliders to
   -- account for the collidable tiles we have. If the collision
   -- portion of the map is ever subtracted from then you should
   -- probably rebuild the colliders
   local tile_rect = function(row, col)
      return Rect((col - 1) * map.tile_width,
                  (row - 1) * map.tile_height,
                  col * map.tile_width,
                  row * map.tile_height)
   end

   local is_solid = function(row, col)
      return map:fetch(row, col).solid
   end

   -- build the colliders
   local current_collider = nil;
   local colliders = {}

   local finish_collider = function()
      local c = util.stage_collidable(current_collider)
      table.insert(colliders, c)
      current_collider = nil
   end

   for row=1,map.height do
      for col=1,map.width do
         if is_solid(row, col) then
            local tr = tile_rect(row, col)
            if current_collider then
               current_collider = current_collider:union(tr)
            else
               current_collider = tr
            end
         elseif current_collider then
            finish_collider()
         end
      end

      if current_collider then
         finish_collider()
      end
   end

   -- remember our colliders in case we need to remove them
   map.colliders = colliders
   return map
end

return M
