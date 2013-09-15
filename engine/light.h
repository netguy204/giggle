#ifndef LIGHT_H
#define LIGHT_H

#include "mesh.h"

class LightCaster : public Object {
public:
  OBJECT_PROTO(LightCaster);
  LightCaster(void* _world);

  void compute_light_mesh(Mesh* result, Walls* walls, const Vector_& light, const Color& c);
};

#endif
