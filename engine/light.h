#ifndef LIGHT_H
#define LIGHT_H

#include "mesh.h"

class LightCaster : public Object {
public:
  OBJECT_PROTO(LightCaster);
  LightCaster(void* _game);

  void compute_light_mesh(Mesh* result, Walls* walls, const Vector_& light);
  void add_triangle(Mesh* result, const Vector_& ref, Wall* w, float a1, float a2);

  Color min_color;
  Color max_color;
  float max_range;
  float angle;
  float halfwidth;
  float max_angle_step;
};

#endif
