local oo = require 'oo'
local util = require 'util'

local Stash = oo.class(oo.Object)

function Stash:init()
end

function Stash:table()
   if not self._cache then
      self._cache = util.deserialize(universe:stash()) or {}
   end
   return self._cache
end

function Stash:save()
   universe:stash(util.serialize(self:table()))
end

function Stash:invalidate()
   self._cache = nil
end

function Stash:update(values)
   local tbl = self:table()
   for k, v in pairs(values) do
      tbl[k] = v
   end
   self:save()
end

function Stash:set(key, value)
   self:update({[key] = value})
end

function Stash:get(key, default)
   local tbl = self:table()
   return tbl[key] or default
end

function Stash:increment(key)
   local v = self:get(key, 1) + 1
   self:set(key, v)
   return v
end

-- there can only be one
return Stash()
