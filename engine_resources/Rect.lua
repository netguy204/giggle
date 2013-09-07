local oo = require 'oo'

local Rect = oo.class(oo.Object)
local vector = require 'vector'

function Rect:init(minx, miny, maxx, maxy)
   if type(minx) == 'number' then
      self.minx = minx
      self.miny = miny
      self.maxx = maxx
      self.maxy = maxy
   else
      self.minx = minx[1]
      self.miny = minx[2]
      self.maxx = minx[3]
      self.maxy = minx[4]
   end
end

-- rect is minx, miny, maxx, maxy
function Rect:union(b)
   return Rect(math.min(self.minx, b.minx),
               math.min(self.miny, b.miny),
               math.max(self.maxx, b.maxx),
               math.max(self.maxy, b.maxy))
end

function Rect:shift(off)
   return Rect(self.minx + off[1],
               self.miny + off[2],
               self.maxx + off[1],
               self.maxy + off[2])
end

function Rect:expand(dims)
   return Rect(self.minx - dims[1]/2,
               self.miny - dims[2]/2,
               self.maxx + dims[1]/2,
               self.maxy + dims[2]/2)
end

function Rect:center()
   return vector.new({(self.minx + self.maxx) / 2,
                      (self.miny + self.maxy) / 2})
end

function Rect.centered(p, w, h)
   return Rect(p[1] - w/2, p[2] - h/2,
               p[1] + w/2, p[2] + h/2)
end

function Rect:width()
   return self.maxx - self.minx
end

function Rect:height()
   return self.maxy - self.miny
end

function Rect:bl()
   return vector.new({self.minx, self.miny})
end

function Rect:br()
   return vector.new({self.maxx, self.miny})
end

function Rect:tl()
   return vector.new({self.minx, self.maxy})
end

function Rect:tr()
   return vector.new({self.maxx, self.maxy})
end

function Rect:intersect(b)
   if (self.maxx < b.minx) or (self.minx > b.maxx) or (self.maxy < b.miny) or (self.miny > b.maxy) then
      return false
   else
      return true
   end
end

function Rect.corners(a, b)
   return Rect(math.min(a[1], b[1]),
               math.min(a[2], b[2]),
               math.max(a[1], b[1]),
               math.max(a[2], b[2]))
end

function Rect:contains(p)
   return p[1] > self.minx and p[1] < self.maxx and p[2] > self.miny and p[2] < self.maxy
end

return Rect
