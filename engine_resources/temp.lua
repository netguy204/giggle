local M = {}

-- rect is minx, miny, maxx, maxy
function M.union(a, b)
   return {
      math.min(a[1], b[1]),
      math.min(a[2], b[2]),
      math.max(a[3], b[3]),
      math.max(a[4], b[4])
   }
end

function M.shift(r, off)
   return { r[1] + off[1], r[2] + off[2],
            r[3] + off[1], r[4] + off[2] }
end

function M.center(a)
   return {(a[1] + a[3]) / 2,
           (a[2] + a[4]) / 2}
end

function M.centered(p, w, h)
   return { p[1] - w/2, p[2] - h/2,
            p[1] + w/2, p[2] + h/2 }
end

function M.width(a)
   return a[3] - a[1]
end

function M.height(a)
   return a[4] - a[2]
end

function M.bl(r)
   return { r[1], r[2] }
end

function M.br(r)
   return { r[3], r[2] }
end

function M.tl(r)
   return { r[1], r[4] }
end

function M.tr(r)
   return { r[3], r[4] }
end

function M.intersect(a, b)
   if (a[3] < b[1]) or (a[1] > b[3]) or (a[4] < b[2]) or (a[2] > b[4]) then
      return false
   else
      return true
   end
end

function M.corners(a, b)
   return {
      math.min(a[1], b[1]),
      math.min(a[2], b[2]),
      math.max(a[1], b[1]),
      math.max(a[2], b[2])
   }
end

return M
