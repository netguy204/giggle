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

struct WallClosestCompare {
  Vector_ pos;
  WallClosestCompare(const Vector_& pos)
    : pos(pos) {
  }

  bool operator()(const Wall* a, const Wall* b) {
    Vector_ ac, bc;
    wall_center(&ac, a);
    wall_center(&bc, b);

    return vector_dist2(&pos, &ac) < vector_dist2(&pos, &bc);
  }
};

void add_wall(Wall** open, long* nwalls, Wall* needle) {
  open[(*nwalls)++] = needle;
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

void LightCaster::compute_light_mesh(Mesh* result, Walls* walls, const Vector_& light) {
  long nwalls = walls->nwalls();

  WallPoint* wall_points = (WallPoint*)frame_alloc(sizeof(WallPoint) * nwalls * 2);
  Wall** open_set = (Wall**)frame_alloc(sizeof(Wall*) * nwalls);
  long nopen_set = 0;

  for(long ii = 0; ii < nwalls; ++ii) {
    Wall* wall = &walls->walls[ii];
    float angle_start = atan2f(wall->start.x - light.x, wall->start.y - light.y);
    float angle_end = atan2f(wall->end.x - light.x, wall->end.y - light.y);

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
  float beginAngle = wall_points[0].angle;

  for(long ii = 0; ii < nwalls * 2; ++ii) {
    WallPoint* wp = &wall_points[ii];
    Wall* last_closest = (nopen_set == 0) ? NULL : open_set[0];
    if(wp->begin) {
      add_wall(open_set, &nopen_set, wp->wall);
    } else {
      remove_wall(open_set, &nopen_set, wp->wall);
    }

    std::sort(open_set, open_set + nopen_set, closest_compare);
    Wall* next_closest = (nopen_set == 0) ? NULL : open_set[0];
    if(last_closest != next_closest) {
      float nextAngle = wp->angle;
      if(last_closest) {
        // interset the discovered angle range with the previously
        // closest wall segment
        Vector_ a1 = {.x = light.x + sinf(beginAngle),
                      .y = light.y + cosf(beginAngle)};
        Vector_ a2 = {.x = light.x + sinf(nextAngle),
                      .y = light.y + cosf(nextAngle)};
        Vector_ i1, i2;
        line_intersection(&i1, &last_closest->start, &last_closest->end, &light, &a1);
        line_intersection(&i2, &last_closest->start, &last_closest->end, &light, &a2);

        // add a triangle to the mesh
        Color white = {1,1,1,0.3};
        result->add_point(light, white);
        result->add_point(i1, white);
        result->add_point(i2, white);
      }
      beginAngle = nextAngle;
    }
  }
}
OBJECT_METHOD(LightCaster, compute_light_mesh, void, (Mesh*, Walls*, Vector_));
