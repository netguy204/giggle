#include "light.h"
#include "giggle.h"

#include <algorithm>

OBJECT_IMPL(LightCaster, Object);
OBJECT_PROPERTY(LightCaster, min_color);
OBJECT_PROPERTY(LightCaster, max_color);
OBJECT_PROPERTY(LightCaster, max_range);
OBJECT_PROPERTY(LightCaster, angle);
OBJECT_PROPERTY(LightCaster, halfwidth);
OBJECT_PROPERTY(LightCaster, max_angle_step);

LightCaster::LightCaster(void* _game)
  : max_range(1e7), angle(0), halfwidth(M_PI), max_angle_step(2*M_PI) {
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

// restrict angle to lie between -PI and PI
static float restrict_angle(float angle) {
  while(angle <= -M_PI) angle += (2 * M_PI);
  while(angle > M_PI) angle -= (2 * M_PI);
  return angle;
}

void LightCaster::compute_light_mesh(Mesh* result, Walls* walls, const Vector_& light) {
  long nwalls = walls->nwalls();
  angle = restrict_angle(angle);

  WallPoint* wall_points = (WallPoint*)GIGGLE->renderer->alloc(sizeof(WallPoint) * nwalls * 2);
  Wall** open_set = (Wall**)GIGGLE->renderer->alloc(sizeof(Wall*) * nwalls);
  long nopen_set = 0;

  for(long ii = 0; ii < nwalls; ++ii) {
    Wall* wall = &walls->walls[ii];
    float angle_start = atan2f(wall->start.y - light.y, wall->start.x - light.x);
    float angle_end = atan2f(wall->end.y - light.y, wall->end.x - light.x);

    float dangle = restrict_angle(angle_end - angle_start);

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
            add_triangle(result, light, last_closest, beginAngle, nextAngle);
          }
          beginAngle = nextAngle;
        }
      }
    }
  }
}
OBJECT_METHOD(LightCaster, compute_light_mesh, void, (Mesh*, Walls*, Vector_));

void color_lerp(Color* out, const Color* a, const Color* b, float s) {
  float s1 = 1 - s;
  out->r = a->r * s1 + b->r * s;
  out->g = a->g * s1 + b->g * s;
  out->b = a->b * s1 + b->b * s;
  out->a = a->a * s1 + b->a * s;
}

// answer is positive if the shortest distance from a to b is
// counterclockwise
static float signed_angular_distance(float a, float b) {
  float diff = b - a;
  if(diff >= M_PI) return -(2*M_PI - diff);
  if(diff < -M_PI) return 2*M_PI + diff;
  return diff;
}

static int sign(float v) {
  if(v >= 0) {
    return 1;
  } else {
    return -1;
  }
}

// returns true if further calls could possibly result in a mesh
void LightCaster::add_triangle(Mesh* result, const Vector_& light,
                               Wall* last_closest, float beginAngle, float nextAngle) {
  // within our angle bounds?
  float da1 = signed_angular_distance(angle, beginAngle);
  if(da1 > halfwidth) return; // swath begins after our cutoff
  float da2 = signed_angular_distance(angle, nextAngle);
  if(da2 < -halfwidth) return; // swath ends before our cutoff

  // clamp the values we did get
  float ada1 = ABS(da1);
  if(ada1 > halfwidth) {
    float fixup = halfwidth - ada1;
    beginAngle += fixup * sign(da1);
  }

  float ada2 = ABS(da2);
  if(ada2 > halfwidth) {
    float fixup = halfwidth - ada2;
    nextAngle += fixup * sign(da2);
  }


  int steps = ceilf(signed_angular_distance(beginAngle, nextAngle) / max_angle_step);

  float oldBegin = beginAngle;
  float oldNext = nextAngle;

  for(int ii = 0; ii < steps; ++ii) {
    beginAngle = oldBegin + ii * max_angle_step;
    float delta = signed_angular_distance(beginAngle, oldNext);
    nextAngle = beginAngle + MIN(delta, max_angle_step);

    // interset the discovered angle range with the previously
    // closest wall segment
    Vector_ a1;
    a1.x = light.x + cosf(beginAngle);
    a1.y = light.y + sinf(beginAngle);
    Vector_ a2;
    a2.x = light.x + cosf(nextAngle);
    a2.y = light.y + sinf(nextAngle);
    Vector_ i1, i2;
    line_intersection(&i1, &last_closest->start, &last_closest->end, &light, &a1);
    line_intersection(&i2, &last_closest->start, &last_closest->end, &light, &a2);

    Color c1, c2;
    float d1 = vector_dist(&i1, &light);
    float d2 = vector_dist(&i2, &light);
    color_lerp(&c1, &min_color, &max_color, MIN(1.0f, d1 / max_range));
    color_lerp(&c2, &min_color, &max_color, MIN(1.0f, d2 / max_range));

    if(d1 > max_range) {
      vector_direction_scaled(&i1, &i1, &light, max_range);
      vector_add(&i1, &i1, &light);
    }

    if(d2 > max_range) {
      vector_direction_scaled(&i2, &i2, &light, max_range);
      vector_add(&i2, &i2, &light);
    }

    // add a triangle to the mesh
    result->add_point(light, min_color);
    result->add_point(i1, c1);
    result->add_point(i2, c2);
  }
}
