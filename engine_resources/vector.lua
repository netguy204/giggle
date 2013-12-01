local M = {}

function M.new(v)
   if v.x then
      v = {v.x, v.y}
   end
   setmetatable(v, M)
   return v
end

M.__index = M

function M.__add(a, b)
   return M.new{ a[1] + b[1], a[2] + b[2] }
end

function M.__sub(a, b)
   return M.new{ a[1] - b[1], a[2] - b[2] }
end

function M.__tostring(a)
   return string.format("<%f, %f>", a[1], a[2])
end

function M.negate(a)
   return M.new{-a[1], -a[2]}
end

function M.norm(a)
   local len = a:length()
   return M.new{ a[1] / len, a[2] / len }
end

function M.length(v)
   return math.sqrt(v[1] * v[1] + v[2] * v[2])
end

function M.dist(a, b)
   local dx = a[1] - b[1]
   local dy = a[2] - b[2]
   return math.sqrt(dx * dx + dy * dy)
end

function M.__mul(a, s)
   return M.new{ a[1] * s, a[2] * s }
end

function M.__div(a, s)
   return M.new{ a[1] / s, a[2] / s }
end

function M.dot(a, b)
   return a[1] * b[1] + a[2] * b[2]
end

function M.angle(a)
   return math.atan2(a[2], a[1])
end

function M.new_from_angle(a)
   return M.new({math.cos(a), math.sin(a)})
end

-- util depends on us so we can't pull this in from there like we'd
-- prefer
local function round(x)
   if x%2 ~= 0.5 then
      return math.floor(x+0.5)
   end
   return x-0.5
end

function M.round(v)
   return M.new({round(v[1]), round(v[2])})
end

return M
