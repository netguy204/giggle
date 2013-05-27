#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include "ooc.h"
#include "color.h"
#include "texture.h"
#include "rect.h"

class FrameBuffer {
 public:
  FrameBuffer();

  Texture* color_buffer;
  unsigned int fbo;
};

class Compositor : public Object {
 public:
  OBJECT_PROTO(Compositor);
  Compositor(void* _emtpy);

  void clear_with_color(Color color);

  Texture* texture_create(int width, int height, int filter);
  void texture_init(Texture* tex, int width, int height, int filter);
  void texture_destroy(Texture* tex);

  FrameBuffer* frame_buffer_create(Texture* color);
  void frame_buffer_init(FrameBuffer* fb, Texture* color);
  void frame_buffer_destroy(FrameBuffer* fb);
  void frame_buffer_bind(FrameBuffer* fb); // null means the default frame buffer

  void textured_quad(Rect_ rect, Color color, Texture* texture);
};

template<>
inline void LCpush<Texture*>(lua_State* L, Texture* tex) {
  lua_pushlightuserdata(L, tex);
}

template<>
inline void LCcheck<Texture*>(lua_State* L, Texture** tex, int pos) {
  *tex = (Texture*)lua_touserdata(L, pos);
}

template<>
inline void LCpush<FrameBuffer*>(lua_State* L, FrameBuffer* fb) {
  lua_pushlightuserdata(L, fb);
}

template<>
inline void LCcheck<FrameBuffer*>(lua_State* L, FrameBuffer** fb, int pos) {
  *fb = (FrameBuffer*)lua_touserdata(L, pos);
}

#endif
