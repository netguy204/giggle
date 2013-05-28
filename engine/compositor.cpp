#include "compositor.h"
#include "testlib.h"
#include "testlib_gl.h"

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

OBJECT_IMPL(TextureObject, Object);

TextureObject::TextureObject(void* _texture)
  : texture((Texture*)_texture) {
}

void TextureObject::destroy() {
  DEFERRED_INVOKE(as_renderable, this, deferred_destroy, NULL);
}

void TextureObject::deferred_destroy() {
  delete texture;
  delete this;
}
DEFERRED_OBJECT_METHOD(as_renderable, TextureObject, deferred_destroy, void, ());

OBJECT_IMPL(FrameBuffer, Object);
OBJECT_ACCESSOR(FrameBuffer, color_buffer, get_color_buffer, set_color_buffer);

FrameBuffer::FrameBuffer(void* _empty) {
  fail_exit("default FrameBuffer constructor is not valid");
}

FrameBuffer::FrameBuffer()
  : color_buffer(NULL), fbo(0) {
}

void FrameBuffer::destroy() {
  DEFERRED_INVOKE(as_renderable, this, deferred_destroy, NULL);
  if(color_buffer) {
    color_buffer->release();
  }
}

void FrameBuffer::deferred_destroy() {
  glDeleteFramebuffers(1, &fbo);
  delete this;
}
DEFERRED_OBJECT_METHOD(as_renderable, FrameBuffer, deferred_destroy, void, ());

TextureObject* FrameBuffer::get_color_buffer() {
  return color_buffer;
}

void FrameBuffer::set_color_buffer(TextureObject* tex) {
  if(color_buffer) {
    color_buffer->release();
  }
  color_buffer = (TextureObject*)tex->retain();
}

const char* framebufferstatus(GLenum result) {
  switch(result) {
  case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
    return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
  case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
    return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
  case GL_FRAMEBUFFER_UNSUPPORTED:
    return "GL_FRAMEBUFFER_UNSUPPORTED";
#ifndef ANDROID
  case GL_FRAMEBUFFER_UNDEFINED:
    return "GL_FRAMEBUFFER_UNDEFINED";
  case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
    return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
  case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
    return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
  case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
    return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
#endif
  default:
    return "unknown";
  }
}

OBJECT_IMPL(Compositor, Object);
Compositor::Compositor(void* empty) {
}

void Compositor::clear_with_color(Color color) {
  glClearColor(color.r, color.g, color.b, color.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
DEFERRED_OBJECT_METHOD(as_renderable, Compositor, clear_with_color, void, (Color));

Matrix44* Compositor::transform_create() {
  return new Matrix44(NULL);
}
OBJECT_METHOD(Compositor, transform_create, Matrix44*, ());

void Compositor::transform_set(Matrix44* m) {
  orthographic_projection = *m;
}
DEFERRED_OBJECT_METHOD(as_renderable, Compositor, transform_set, void, (Matrix44*));

TextureObject* Compositor::texture_create(int width, int height, int filter) {
  TextureObject* texture = new TextureObject(new Texture());
  DEFERRED_INVOKE(as_renderable, this, texture_init, texture, width, height, filter);
  texture->reference_count = 0; // disown
  return texture;
}
OBJECT_METHOD(Compositor, texture_create, TextureObject*, (int, int, int));

void Compositor::texture_init(TextureObject* texture, int width, int height, int filter) {
  if(texture->texture->texid <= 0) {
    gl_check(glGenTextures(1, &texture->texture->texid));
  }
  texture->texture->bind();
  gl_check(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                        width, height,
                        0, GL_RGB, GL_UNSIGNED_BYTE, NULL));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter));
}
DEFERRED_OBJECT_METHOD(as_renderable, Compositor, texture_init, void, (TextureObject*, int, int, int));

FrameBuffer* Compositor::frame_buffer_create(TextureObject* color) {
  FrameBuffer* fb = new FrameBuffer();
  fb->set_color_buffer(color);

  DEFERRED_INVOKE(as_renderable, this, frame_buffer_init, fb);
  fb->reference_count = 0; // disown
  return fb;
}
OBJECT_METHOD(Compositor, frame_buffer_create, FrameBuffer*, (TextureObject*));

void Compositor::frame_buffer_init(FrameBuffer* fb) {
  if(fb->fbo == 0) {
    gl_check(glGenFramebuffers(1, &fb->fbo));
  }

  gl_check(glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo));
  gl_check(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_2D, fb->color_buffer->texture->texid, 0));

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if(status != GL_FRAMEBUFFER_COMPLETE) {
    fail_exit("frame buffer is not complete: %s", framebufferstatus(status));
  }
  gl_check(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}
DEFERRED_OBJECT_METHOD(as_renderable, Compositor, frame_buffer_init, void, (FrameBuffer*));

void Compositor::frame_buffer_bind(FrameBuffer* fb) {
  if(fb) {
    gl_check(glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo));
  } else {
    gl_check(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  }
}
DEFERRED_OBJECT_METHOD(as_renderable, Compositor, frame_buffer_bind, void, (FrameBuffer*));

void Compositor::textured_quad(Rect_ rect, Color color, TextureObject* texture) {
  Program* program = get_program(standard_color_program_loader);

  GLfloat verts[] = {
    rect.minx, rect.miny, 0.0f, // bl
    rect.maxx, rect.miny, 0.0f, // br
    rect.maxx, rect.maxy, 0.0f, // tr
    rect.maxx, rect.maxy, 0.0f, // tr
    rect.minx, rect.maxy, 0.0f, // tl
    rect.minx, rect.miny, 0.0f // bl
  };

  GLfloat colors[] = {
    color.r, color.g, color.b, color.a,
    color.r, color.g, color.b, color.a,
    color.r, color.g, color.b, color.a,
    color.r, color.g, color.b, color.a,
    color.r, color.g, color.b, color.a,
    color.r, color.g, color.b, color.a
  };

  GLfloat texs[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 1.0f,
    0.0f, 0.0f
  };

  gl_check(program->use());
  GLuint vert_buffer = next_buffer();
  gl_check(glEnableVertexAttribArray(GLPARAM_VERTEX));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, vert_buffer));
  gl_check(glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW));
  gl_check(glVertexAttribPointer(GLPARAM_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0));

  GLuint tex_buffer = next_buffer();
  gl_check(glEnableVertexAttribArray(GLPARAM_TEXCOORD0));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, tex_buffer));
  gl_check(glBufferData(GL_ARRAY_BUFFER, sizeof(texs), texs, GL_DYNAMIC_DRAW));
  gl_check(glVertexAttribPointer(GLPARAM_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0));

  GLuint color_buffer = next_buffer();
  gl_check(glEnableVertexAttribArray(GLPARAM_OTHER0));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, color_buffer));
  gl_check(glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4 * 6, colors, GL_DYNAMIC_DRAW));
  gl_check(glVertexAttribPointer(GLPARAM_OTHER0, 4, GL_FLOAT, GL_FALSE, 0, 0));
  texture->texture->bind();

  gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));
  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE, orthographic_projection.data));
  gl_check(glDrawArrays(GL_TRIANGLES, 0, 6));
}
DEFERRED_OBJECT_METHOD(as_renderable, Compositor, textured_quad, void, (Rect_, Color, TextureObject*))
