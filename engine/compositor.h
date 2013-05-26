#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include "ooc.h"
#include "color.h"

class Compositor : public Object {
 public:
  OBJECT_PROTO(Compositor);
  Compositor(void* _emtpy);

  void clear_with_color(Color color);
};

#endif
