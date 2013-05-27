#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include "ooc.h"
#include "color.h"
#include "texture.h"

class FrameBuffer : public Object {
 public:
  OBJECT_PROTO(FrameBuffer);
  FrameBuffer(void* _emtpy);

  Texture* color_buffer;
};

class Compositor : public Object {
 public:
  OBJECT_PROTO(Compositor);
  Compositor(void* _emtpy);

  void clear_with_color(Color color);
  Texture* texture_create(int width, int height, int filter);
  void texture_init(Texture* tex, int width, int height, int filter);
  void texture_destroy(Texture* tex);
};

template<>
inline void LCpush<Texture*>(lua_State* L, Texture* tex) {
  lua_pushlightuserdata(L, tex);
}

template<>
inline void LCcheck<Texture*>(lua_State* L, Texture** tex, int pos) {
  *tex = (Texture*)lua_touserdata(L, pos);
}


#endif
