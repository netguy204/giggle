#include "compositor.h"
#include "testlib.h"

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
