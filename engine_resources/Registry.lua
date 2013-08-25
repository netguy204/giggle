local oo = require 'oo'
local Registry = oo.class(oo.Object)

function Registry:init()
   self.db = {}
end

function Registry:key(obj)
   return (obj.key and obj:key()) or obj
end

function Registry:register(go, obj)
   self.db[self:key(go)] = obj
end

function Registry:unregister(go)
   self.db[self:key(go)] = nil
end

function Registry:find(go)
   return self.db[self:key(go)]
end

return Registry
