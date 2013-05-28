#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include "ooc.h"
#include "color.h"
#include "texture.h"
#include "rect.h"
#include "gameobject.h"
#include "matrix.h"

class TextureObject : public Object {
 public:
  OBJECT_PROTO(TextureObject);
  TextureObject(void* _texture);

  virtual void destroy();
  void deferred_destroy();

  Texture* texture;
};

class FrameBuffer : public Object {
 public:
  OBJECT_PROTO(FrameBuffer);
  FrameBuffer(void* _empty);
  FrameBuffer();

  virtual void destroy();
  void deferred_destroy();

  void set_color_buffer(TextureObject* tex);
  TextureObject* get_color_buffer();

  TextureObject* color_buffer;
  unsigned int fbo;
};

class Compositor : public Object {
 public:
  OBJECT_PROTO(Compositor);
  Compositor(void* _emtpy);

  void clear_with_color(Color color);

  Matrix44* transform_create();
  void transform_set(Matrix44* mat);

  TextureObject* texture_create(int width, int height, int filter);
  void texture_init(TextureObject* tex, int width, int height, int filter);

  FrameBuffer* frame_buffer_create(TextureObject* color);
  void frame_buffer_init(FrameBuffer* fb);
  void frame_buffer_bind(FrameBuffer* fb); // null means the default frame buffer

  void textured_quad(Rect_ rect, Color color, TextureObject* texture);
};

template<>
inline void LCpush<Compositor*>(lua_State* L, Compositor* compositor) {
  LCpush_lut(L, LUT_COMPOSITOR, compositor);
}

template<>
inline void LCcheck<Compositor*>(lua_State* L, Compositor** compositor, int pos) {
  *compositor = (Compositor*)LCcheck_lut(L, LUT_COMPOSITOR, pos);
}

template<>
inline void LCpush<TextureObject*>(lua_State* L, TextureObject* tex) {
  LCpush_lut(L, LUT_TEXTURE, tex);
}

template<>
inline void LCcheck<TextureObject*>(lua_State* L, TextureObject** tex, int pos) {
  *tex = (TextureObject*)LCcheck_lut(L, LUT_TEXTURE, pos);
}

template<>
inline void LCpush<FrameBuffer*>(lua_State* L, FrameBuffer* fb) {
  LCpush_lut(L, LUT_FRAMEBUFFER, fb);
}

template<>
inline void LCcheck<FrameBuffer*>(lua_State* L, FrameBuffer** fb, int pos) {
  *fb = (FrameBuffer*)LCcheck_lut(L, LUT_FRAMEBUFFER, pos);
}

#endif
