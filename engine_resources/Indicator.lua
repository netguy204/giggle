local oo = require 'oo'
local Registry = require 'Registry'

local Indicator = oo.class(oo.Object)
Indicator.reg = Registry()

function Indicator.with_all(fn)
   for k, obj in pairs(Indicator.reg.db) do
      fn(obj)
   end
end

function Indicator.terminate_all()
   local fn = function(obj)
      obj:terminate()
   end
   Indicator.with_all(fn)
end

function Indicator:init(font, pos, color, go)
   self.font = font
   self.pos = pos
   go = go or stage
   self.text = go:add_component('CDrawText', {font=font, color=color})
   Indicator.reg:register(self, self)
end

function Indicator:update(msg, ...)
   msg = string.format(msg, ...)
   self.text:offset({self.pos[1] - self.font:string_width(msg)/2,
                     self.pos[2] - self.font:line_height()/2})
   self.text:message(msg)
end

function Indicator:color(color)
   self.text:color(color)
end

function Indicator:terminate()
   Indicator.reg:unregister(self)
   self.text:delete_me(1)
end

return Indicator
