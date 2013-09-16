local M = {}

local function native_index(obj, item)
   -- first, try to resolve the call against our overlay object
   local val = obj.__overlay and obj.__overlay[item]
   if val then return val end

   -- otherwise, the native object should get the call with itself as
   -- the first argument instead of the table the call was against
   local fn = function(...)
      local args = {...}
      args[1] = obj.__native
      return obj.__native[item](table.unpack(args))
   end

   return fn
end

local function wrap_native(native, overlay)
   local obj = {native = native, __overlay = overlay}
   local mt = {__index = native_index}
   setmetatable(obj, mt)
   return obj
end


function M.class(base)
   local c = {}    -- a new class instance
   if type(base) == 'table' then
      -- our new class is a shallow copy of the base class!
      for i,v in pairs(base) do
         c[i] = v
      end
      c._base = base
   end
   -- the class will be the metatable for all its objects,
   -- and they will look up their methods in it.
   c.__index = c

   -- expose a constructor which can be called by <classname>(<args>)
   local mt = {}
   mt.__call = function(class_tbl, ...)
      local obj = {}
      setmetatable(obj,c)
      if class_tbl.init then
         class_tbl.init(obj,...)
      else
         -- make sure that any stuff from the base class is initialized!
         if base and base.init then
            base.init(obj, ...)
         end
      end

      -- if the constructed obj includes a __native field then wrap it
      -- as a native overlay
      if obj.__native then
         obj = wrap_native(obj.__native, obj)
         obj.class = function(self)
            return getmetatable(self.__overlay)
         end
         obj.native = function(self)
            return self.__native
         end
      end

      return obj
   end
   c.init = init
   c.class = function(self)
      return getmetatable(self)
   end
   c.is_a = function(self, klass)
      local m = self:class()
      while m do
         if m == klass then return true end
         m = m._base
      end
      return false
   end
   setmetatable(c, mt)
   return c
end

Object = M.class()
M.Object = Object

function Object:bind(fname)
   local mem = self[fname] or error(fname .. ' is not bound')
   local fn = function(...)
      mem(self, ...)
   end
   return fn
end

return M
