#include "compositor.h"
#include "testlib.h"
#include "testlib_gl.h"

OBJECT_IMPL(FrameBuffer, Object);
OBJECT_PROPERTY(FrameBuffer, color_buffer);

FrameBuffer::FrameBuffer(void* _empty) {
}



class VoidFunctionRenderable : public Renderable {
public:
  OBJECT_PROTO(VoidFunctionRenderable);
  VoidFunctionRenderable(void* _fn)
    : Renderable(NULL) {
    fn = (VoidFunction*)_fn;
  }

  virtual void render(void *args) {
    fn->invoke();
  }

  VoidFunction* fn;
};
OBJECT_IMPL(VoidFunctionRenderable, Renderable);

void as_renderable(VoidFunction* fn) {
  void* buffer = frame_alloc(sizeof(VoidFunctionRenderable));
  VoidFunctionRenderable* vfr = new(buffer) VoidFunctionRenderable(fn);
  renderable_enqueue_for_screen(vfr, NULL);
}

OBJECT_IMPL(Compositor, Object);
Compositor::Compositor(void* empty) {
}

void Compositor::clear_with_color(Color color) {
  glClearColor(color.r, color.g, color.b, color.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
DEFERRED_OBJECT_METHOD(as_renderable, Compositor, clear_with_color, void, (Color));

Texture* Compositor::texture_create(int width, int height, int filter) {
  Texture* texture = new Texture();
  DEFERRED_INVOKE(as_renderable, this, texture_init, texture, width, height, filter);
  return texture;
}
OBJECT_METHOD(Compositor, texture_create, Texture*, (int, int, int));

void Compositor::texture_init(Texture* texture, int width, int height, int filter) {
  if(texture->texid <= 0) {
    gl_check(glGenTextures(1, &texture->texid));
  }
  texture->bind();
  gl_check(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                        width, height,
                        0, GL_RGB, GL_UNSIGNED_BYTE, NULL));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter));
}
DEFERRED_OBJECT_METHOD(as_renderable, Compositor, texture_init, void, (Texture*, int, int, int));

void Compositor::texture_destroy(Texture* tex) {
  delete tex;
}
DEFERRED_OBJECT_METHOD(as_renderable, Compositor, texture_destroy, void, (Texture*));
