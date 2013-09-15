#include "light.h"
#include "testlib.h"

#include <algorithm>

OBJECT_IMPL(LightCaster, Object);

LightCaster::LightCaster(void* _world) {
}

struct WallPoint {
  Vector point;
  Wall* wall;
  float angle;
  bool begin;
};

struct WallPointCompare {
  bool operator()(const WallPoint& a, const WallPoint& b) {
    if(a.angle == b.angle) {
      if(a.begin == b.begin) {
        return a.wall < b.wall;
      } else if(a.begin) {
        return true;
      } else {
        return false;
      }
    } else {
      return a.angle < b.angle;
    }
  }
};

void wall_center(Vector center, const Wall* wall) {
  vector_add(center, &wall->start, &wall->end);
  vector_scale(center, center, 0.5);
}

// from: http://www.redblobgames.com/articles/visibility/
// Helper: leftOf(segment, point) returns true if point is
// "left" of segment treated as a vector
bool leftOf(const Wall* s, const Vector_* p) {
  float cross = (s->end.x - s->start.x) * (p->y - s->start.y)
    - (s->end.y - s->start.y) * (p->x - s->start.x);
  return cross < 0;
}

// Return p*(1-f) + q*f
Vector interpolate(Vector result, const Vector_* p, const Vector_* q, float f) {
  result->x = p->x*(1-f) + q->x*f;
  result->y = p->y*(1-f) + q->y*f;
  return result;
}

// Helper: do we know that segment a is in front of b?
// Implementation not anti-symmetric (that is to say,
// _segment_in_front_of(a, b) != (!_segment_in_front_of(b, a)).
// Also note that it only has to work in a restricted set of cases
// in the visibility algorithm; I don't think it handles all
// cases. See http://www.redblobgames.com/articles/visibility/segment-sorting.html
bool _segment_in_front_of(const Wall* a, const Wall* b, const Vector_* relativeTo) {
  // NOTE: we slightly shorten the segments so that
  // intersections of the endpoints (common) don't count as
  // intersections in this algorithm
  Vector_ temp;
  bool A1 = leftOf(a, interpolate(&temp, &b->start, &b->end, 0.01));
  bool A2 = leftOf(a, interpolate(&temp, &b->end, &b->start, 0.01));
  bool A3 = leftOf(a, relativeTo);
  bool B1 = leftOf(b, interpolate(&temp, &a->start, &a->end, 0.01));
  bool B2 = leftOf(b, interpolate(&temp, &a->end, &a->start, 0.01));
  bool B3 = leftOf(b, relativeTo);

  // NOTE: this algorithm is probably worthy of a short article
  // but for now, draw it on paper to see how it works. Consider
  // the line A1-A2. If both B1 and B2 are on one side and
  // relativeTo is on the other side, then A is in between the
  // viewer and B. We can do the same with B1-B2: if A1 and A2
  // are on one side, and relativeTo is on the other side, then
  // B is in between the viewer and A.
  if (B1 == B2 && B2 != B3) return true;
  if (A1 == A2 && A2 == A3) return true;
  if (A1 == A2 && A2 != A3) return false;
  if (B1 == B2 && B2 == B3) return false;

  // If A1 != A2 and B1 != B2 then we have an intersection.
  // Expose it for the GUI to show a message. A more robust
  // implementation would split segments at intersections so
  // that part of the segment is in front and part is behind.
  //demo_intersectionsDetected.push([a.p1, a.p2, b.p1, b.p2]);
  return false;

  // NOTE: previous implementation was a.d < b.d. That's simpler
  // but trouble when the segments are of dissimilar sizes. If
  // you're on a grid and the segments are similarly sized, then
  // using distance will be a simpler and faster implementation.
}


struct WallClosestCompare {
  Vector_ pos;
  WallClosestCompare(const Vector_& pos)
    : pos(pos) {
  }

  bool operator()(const Wall* a, const Wall* b) {
    return _segment_in_front_of(b, a, &pos);
  }
};

void add_wall(Wall** open, long* nwalls, Wall* needle, const Vector_* ref) {
  if(*nwalls == 0) {
    open[(*nwalls)++] = needle;
    return;
  }

  long lower = 0;
  long upper = *nwalls - 1;
  long found_idx = -1;

  if(!_segment_in_front_of(needle, open[lower], ref)) {
    found_idx = lower;
  } else if(_segment_in_front_of(needle, open[upper], ref)) {
    // must go at the end
    open[(*nwalls)++] = needle;
    return;
  } else {
    // bisect for it
    while(upper - lower > 1) {
      long idx = (lower + upper) / 2;
      if(!_segment_in_front_of(needle, open[idx], ref)) {
        upper = idx;
      } else {
        lower = idx;
      }
    }
    found_idx = upper;
  }

  // FIXME
  if(found_idx == -1) fail_exit("something bad happened");

  // insert and shift everything up
  Wall* temp = open[found_idx];
  open[found_idx] = needle;
  for(long jj = found_idx + 1; jj < *nwalls; ++jj) {
    Wall* next_temp = open[jj];
    open[jj] = temp;
    temp = next_temp;
  }
  open[(*nwalls)++] = temp;
}

bool remove_wall(Wall** open, long* nwalls, Wall* needle) {
  for(long ii = 0; ii < *nwalls; ++ii) {
    if(open[ii] == needle) {
      // back fill from here
      for(long jj = ii; jj < (*nwalls - 1); ++jj) {
        open[jj] = open[jj + 1];
      }
      (*nwalls)--;
      return true;
    }
  }
  return false;
}

void line_intersection(Vector result, const Vector_* p1, const Vector_* p2,
                       const Vector_* p3, const Vector_* p4) {
  // From http://paulbourke.net/geometry/lineline2d/
  float s = ((p4->x - p3->x) * (p1->y - p3->y) - (p4->y - p3->y) * (p1->x - p3->x))
    / ((p4->y - p3->y) * (p2->x - p1->x) - (p4->x - p3->x) * (p2->y - p1->y));
  result->x = p1->x + s * (p2->x - p1->x);
  result->y = p1->y + s * (p2->y - p1->y);
}

void LightCaster::compute_light_mesh(Mesh* result, Walls* walls,
                                     const Vector_& light, const Color& color) {
  long nwalls = walls->nwalls();

  WallPoint* wall_points = (WallPoint*)frame_alloc(sizeof(WallPoint) * nwalls * 2);
  Wall** open_set = (Wall**)frame_alloc(sizeof(Wall*) * nwalls);
  long nopen_set = 0;

  for(long ii = 0; ii < nwalls; ++ii) {
    Wall* wall = &walls->walls[ii];
    float angle_start = atan2f(wall->start.y - light.y, wall->start.x - light.x);
    float angle_end = atan2f(wall->end.y - light.y, wall->end.x - light.x);

    float dangle = angle_end - angle_start;
    if(dangle <= -M_PI) dangle += (2*M_PI);
    if(dangle > M_PI) dangle -= (2*M_PI);

    long idx1 = 2*ii;
    long idx2 = idx1 + 1;
    wall_points[idx1].point = &wall->start;
    wall_points[idx1].wall = wall;
    wall_points[idx1].angle = angle_start;
    wall_points[idx1].begin = dangle > 0;

    wall_points[idx2].point = &wall->end;
    wall_points[idx2].wall = wall;
    wall_points[idx2].angle = angle_end;
    wall_points[idx2].begin = !wall_points[idx1].begin;
  }

  std::sort(wall_points, wall_points + (nwalls*2), WallPointCompare());

  WallClosestCompare closest_compare(light);
  float beginAngle = 0;

  for(int kk = 0; kk < 2; ++kk) {
    for(long ii = 0; ii < nwalls * 2; ++ii) {
      WallPoint* wp = &wall_points[ii];
      Wall* last_closest = (nopen_set == 0) ? NULL : open_set[0];
      if(wp->begin) {
        add_wall(open_set, &nopen_set, wp->wall, &light);
      } else {
        remove_wall(open_set, &nopen_set, wp->wall);
      }

      Wall* next_closest = (nopen_set == 0) ? NULL : open_set[0];
      if(last_closest != next_closest) {
        float nextAngle = wp->angle;

        if(last_closest) {
          if(kk == 1) {
            // interset the discovered angle range with the previously
            // closest wall segment
            Vector_ a1 = {.x = light.x + cosf(beginAngle),
                          .y = light.y + sinf(beginAngle)};
            Vector_ a2 = {.x = light.x + cosf(nextAngle),
                          .y = light.y + sinf(nextAngle)};
            Vector_ i1, i2;
            line_intersection(&i1, &last_closest->start, &last_closest->end, &light, &a1);
            line_intersection(&i2, &last_closest->start, &last_closest->end, &light, &a2);

            // add a triangle to the mesh
            result->add_point(light, color);
            result->add_point(i1, color);
            result->add_point(i2, color);
          }
          beginAngle = nextAngle;
        }
      }
    }
  }
}
OBJECT_METHOD(LightCaster, compute_light_mesh, void, (Mesh*, Walls*, Vector_, Color));
