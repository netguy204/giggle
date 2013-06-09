local oo = require 'oo'
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

return Registry
