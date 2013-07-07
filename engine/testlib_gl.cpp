/*
 *  This file is part of GambitGameLib.
 *
 *  GambitGameLib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GambitGameLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GambitGameLib.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "gl_headers.h"
#include "testlib_gl.h"

#include "memlib.h"
#include "utils.h"
#include "testlib.h"
#include "utils.h"

#include <math.h>
#include <stdio.h>
#include <stdarg.h>

#define NBUFFERS 30

StackAllocator gldata_allocator = NULL;
float gl_version = 0.0f;
int fbo_support = 0;
GLuint buffers[NBUFFERS];
int current_buffer = 0;

void* renderer_alloc(long size) {
  return stack_allocator_alloc(gldata_allocator, size);
}

void gl_check_(const char * msg) {
  GLenum error = glGetError();
  if(error == GL_NO_ERROR) return;

  const char* e_msg;
  switch(error) {
  case GL_INVALID_ENUM:
    e_msg = "GL_INVALID_ENUM";
    break;
  case GL_INVALID_VALUE:
    e_msg = "GL_INVALID_VALUE";
    break;
  case GL_INVALID_OPERATION:
    e_msg = "GL_INVALID_OPERATION";
    break;
  case GL_OUT_OF_MEMORY:
    e_msg = "GL_OUT_OF_MEMORY";
    break;
  default:
    e_msg = "unknown";
  }

  LOGW("GL_ERROR: %s => %s\n", msg, e_msg);
}

GLuint next_buffer() {
  GLuint buffer = buffers[current_buffer];
  current_buffer = (current_buffer + 1) % NBUFFERS;
  return buffer;
}

GLMemory::GLMemory()
  : data(NULL), buffer(0) {
}

GLMemory* gl_bufinit(GLMemory* mem) {
  mem->buffer = next_buffer();
  return mem;
}

GLMemory* gl_claim(GLMemory* mem, size_t sz) {
  mem->size = sz;
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, mem->buffer));
  gl_check(glBufferData(GL_ARRAY_BUFFER, sz, NULL, GL_DYNAMIC_DRAW));
  mem->data = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
  gl_check_("glMapBuffer");
  return mem;
}

void gl_unclaim(GLMemory* mem) {
  if(mem->data) {
    gl_check(glBindBuffer(GL_ARRAY_BUFFER, mem->buffer));
    gl_check(glUnmapBuffer(GL_ARRAY_BUFFER));
    mem->data = NULL;
  }
}

char* shader_buffer = NULL;

int renderer_load_shader(const char* src, GLenum kind) {
  int shader = glCreateShader(kind);
  gl_check_("glCreateShader");

  const int max_shader = 8 * 1024;
  if(!shader_buffer) {
    shader_buffer = (char*)malloc(max_shader);
  }

#ifndef ANDROID
  snprintf(shader_buffer, max_shader, "#version 120\n%s", src);
#else
  snprintf(shader_buffer, max_shader, "%s", src);
#endif

  glShaderSource(shader, 1, (const char**)&shader_buffer, NULL);
  gl_check_("glShaderSource");

  glCompileShader(shader);
  gl_check_("glCompileShader");

  int status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

  if(status == GL_FALSE) {
    char buffer[1024];
    int length;
    glGetShaderInfoLog(shader, sizeof(buffer), &length, buffer);
    fail_exit("glCompileShader: %s, %s\n", buffer, src);
  }

  return shader;
}

Program* renderer_link_shader(const char* vertexname, const char* fragmentname, ...) {
  char* vertex_source = filename_slurp(vertexname);
  char* fragment_source = filename_slurp(fragmentname);

  LOGI("renderer_load_shader: %s", vertexname);
  int vertex = renderer_load_shader(vertex_source, GL_VERTEX_SHADER);
  LOGI("renderer_load_shader: %s", fragmentname);
  int fragment = renderer_load_shader(fragment_source, GL_FRAGMENT_SHADER);
  free(vertex_source);
  free(fragment_source);

  int program = glCreateProgram();

  gl_check(glAttachShader(program, vertex));
  gl_check(glAttachShader(program, fragment));

  va_list ap;
  va_start(ap, fragmentname);
  while(1) {
    ProgramParameters param = (ProgramParameters)va_arg(ap, int);
    if(param == GLPARAM_DONE) break;

    const char* name = va_arg(ap, char*);
    gl_check(glBindAttribLocation(program, param, name));
  }

  gl_check(glLinkProgram(program));

  gl_check(glDeleteShader(vertex));
  gl_check(glDeleteShader(fragment));
  int link_status;
  glGetProgramiv(program, GL_LINK_STATUS, &link_status);
  if(link_status == GL_FALSE) {
    char buffer[1024];
    int length;
    glGetProgramInfoLog(program, sizeof(buffer), &length, buffer);
    fail_exit("glLinkProgram: %s\n", buffer);
  }

  Program* p = new Program();
  p->program = program;
  return p;
}

void program_bind_uniforms(Program* p, ...) {
  va_list ap;
  va_start(ap, p);
  while(1) {
    ProgramUniforms uniform = (ProgramUniforms)va_arg(ap, int);
    if(uniform == UNIFORM_DONE) break;

    const char* name = va_arg(ap, char*);
    GLint loc = glGetUniformLocation(p->program, name);
    if(loc < 0) {
      fail_exit("glGetUniformLocation: %s error %d", name, loc);
    }

    p->uniforms[uniform] = loc;
  }
}


Program::Program() {
  program = -1;
  for(int ii = 0; ii < UNIFORM_DONE; ++ii) {
    uniforms[ii] = -1;
  }
}

Program::~Program() {
  if(program != -1) {
    glDeleteProgram(program);
  }
}

GLuint Program::current_program = 0;

LoaderToProgram programs;
Program* get_program(ProgramLoader loader) {
  LoaderToProgram::iterator iter = programs.find(loader);
  if(iter == programs.end()) {
    Program* program = loader();
    programs.insert(std::make_pair(loader, program));
    return program;
  } else {
    return iter->second;
  }
}

Program* standard_program_loader() {
  Program *program = renderer_link_shader("engine_resources/standard.vert",
                                          "engine_resources/standard.frag",
                                          GLPARAM_VERTEX, "vertex",
                                          GLPARAM_TEXCOORD0, "tex_coord0",
                                          GLPARAM_DONE);
  program_bind_uniforms(program,
                        UNIFORM_TEX0, "textureUnit0",
                        UNIFORM_MVP, "mvpMatrix",
                        UNIFORM_DONE);

  return program;
}

Program* solid_program_loader() {
  Program *program = renderer_link_shader("engine_resources/standard.vert",
                                          "engine_resources/solid.frag",
                                          GLPARAM_VERTEX, "vertex",
                                          GLPARAM_DONE);
  program_bind_uniforms(program,
                        UNIFORM_MVP, "mvpMatrix",
                        UNIFORM_COLOR0, "color",
                        UNIFORM_DONE);
  return program;
}

Program* standard_color_program_loader() {
  Program *program = renderer_link_shader("engine_resources/standard_color.vert",
                                          "engine_resources/standard_color.frag",
                                          GLPARAM_VERTEX, "vertex",
                                          GLPARAM_TEXCOORD0, "tex_coord0",
                                          GLPARAM_OTHER0, "color_coord0",
                                          GLPARAM_DONE);
  program_bind_uniforms(program,
                        UNIFORM_TEX0, "textureUnit0",
                        UNIFORM_MVP, "mvpMatrix",
                        UNIFORM_DONE);
  return program;
}

Program* tile_program_loader() {
  Program *program = renderer_link_shader("engine_resources/tile.vert",
                                          "engine_resources/tile.frag",
                                          GLPARAM_VERTEX, "vertex",
                                          GLPARAM_TEXCOORD0, "tex_coord0",
                                          GLPARAM_DONE);
  program_bind_uniforms(program,
                        UNIFORM_TEX0, "textureUnit0",
                        UNIFORM_MVP, "mvpMatrix",
                        UNIFORM_LIGHT0_POSITION, "light",
                        UNIFORM_DONE);
  return program;
}

Matrix44 orthographic_projection(NULL);

Renderable* basespritelist_renderer;
Renderable* spritelist_renderer;
Renderable* coloredspritelist_renderer;
Renderable* rect_renderer;
Renderable* filledrect_renderer;

void renderer_init_standard_shader() {
  basespritelist_renderer = new BaseSpriteListRenderer(NULL);
  spritelist_renderer = new SpriteListRenderer(NULL);
  coloredspritelist_renderer = new ColoredSpriteListRenderer(NULL);
  rect_renderer = new RectRenderer(NULL);
  filledrect_renderer = new FilledRectRenderer(NULL);
}

int real_screen_width;
int real_screen_height;

void renderer_resize(int w, int h) {
  real_screen_width = w;
  real_screen_height = h;

  screen_width = w;
  screen_height = h;
  glViewport(0, 0, screen_width, screen_height);
  orthographic_projection.orthographic_proj(0.0f, screen_width, 0.0f, screen_height,
                                            -1.0f, 1.0f);
}

void renderer_gl_init(int w, int h) {
  LOGI("renderer_gl_init");

  // determine opengl version
  gl_version = atof((char*)glGetString(GL_VERSION));
  LOGI("opengl version %f", gl_version);

#ifdef ANDROID
  fbo_support = 1;
#else
  if((void*)glGenFramebuffers != NULL) {
    fbo_support = 1;
  }
#endif

  if(!gldata_allocator) {
    gldata_allocator = stack_allocator_make(1024 * 1024, "gldata_allocator");
  }

  glEnable(GL_TEXTURE_2D);
  gl_check_("GL_TEXTURE_2D");
  glEnable(GL_BLEND);
  gl_check_("GL_BLEND");
  // valid point sprite sizes are from 1.0 - 63.0
#ifndef ANDROID
  glEnable(GL_POINT_SPRITE);
  gl_check_("GL_POINT_SPRITE");
#endif

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  gl_check(glGenBuffers(NBUFFERS, buffers));

  float r = 240.0f / 255.0f;
  float g = 240.0f / 255.0f;
  float b = 225.0f / 255.0f;
  glClearColor(r, g, b, 1.0f);

  renderer_resize(w, h);

  // build a vao that we'll use for everything
  GLuint vao;
#ifdef __APPLE__
  gl_check(glGenVertexArraysAPPLE(1, &vao));
  gl_check(glBindVertexArrayAPPLE(vao));
#else
#ifdef __ANDROID__
  //glGenVertexArraysOES(1, &vao);
  //glBindVertexArrayOES(vao);
#else
#ifndef WINDOWS
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
#endif
#endif
#endif
  gl_check_("vertex arrays");

  renderer_init_standard_shader();

  gl_check_("setup");
}

void renderer_gl_shutdown() {
  glClear(GL_COLOR_BUFFER_BIT);
}

Timer_ rtimer;

void renderer_begin_frame(void* empty) {
  PROFILE_START(&rtimer, "renderer");
  stack_allocator_freeall(gldata_allocator);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderer_end_frame() {
  PROFILE_END(&rtimer);
}

void renderer_finish_image_load(ImageResource resource) {
  LOGI("finish image load");
  GLenum texture_format;
  GLint num_colors;

  num_colors = resource->channels;
  if(num_colors == 4) {
    texture_format = GL_RGBA;
  } else {
    texture_format = GL_RGB;
  }

  gl_check(glGenTextures(1, &resource->texture->texid));
  resource->texture->bind();

  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
  gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
  gl_check(glTexImage2D(GL_TEXTURE_2D, 0, texture_format,
                        resource->w, resource->h, 0,
                        texture_format, GL_UNSIGNED_BYTE, resource->data));

  free(resource->data);
}

void renderer_finish_image_free(void* texturep) {
  Texture* texture = (Texture*)texturep;
  delete texture;
}

void spritelist_set_texs_and_verts_gl(int nverts, GLMemory* verts, GLMemory* texs) {
  gl_check(glEnableVertexAttribArray(GLPARAM_VERTEX));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, verts->buffer));
  gl_check(glVertexAttribPointer(GLPARAM_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0));
  gl_unclaim(verts);

  gl_check(glEnableVertexAttribArray(GLPARAM_TEXCOORD0));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, texs->buffer));
  gl_check(glVertexAttribPointer(GLPARAM_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, 0, 0));
  gl_unclaim(texs);
}

int basespritelist_set(BaseSprite list, GLMemory* vertmem, GLMemory* texmem) {
  if(!list) return 0;
  int nquads = list->count;
  int ntris = nquads * 2;
  int nverts = 3 * ntris;

  gl_claim(vertmem, sizeof(float) * nverts * 3);
  gl_claim(texmem, sizeof(float) * nverts * 2);

  GLfloat* verts = (GLfloat*)vertmem->data;
  GLfloat* texs = (GLfloat*)texmem->data;

  int vert_idx = 0;
  int tex_idx = 0;

  BaseSprite sprite;
  for(sprite = list; sprite != NULL;
      sprite = sprite->next) {

    // bottom-left
    verts[vert_idx++] = sprite->displayX;
    verts[vert_idx++] = sprite->displayY;
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u0;
    texs[tex_idx++] = sprite->v0;

    // bottom-right
    verts[vert_idx++] = sprite->displayX + sprite->w;
    verts[vert_idx++] = sprite->displayY;
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u1;
    texs[tex_idx++] = sprite->v0;

    // top-right
    verts[vert_idx++] = sprite->displayX + sprite->w;
    verts[vert_idx++] = sprite->displayY + sprite->h;
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u1;
    texs[tex_idx++] = sprite->v1;

    // top-right
    verts[vert_idx++] = sprite->displayX + sprite->w;
    verts[vert_idx++] = sprite->displayY + sprite->h;
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u1;
    texs[tex_idx++] = sprite->v1;

    // top-left
    verts[vert_idx++] = sprite->displayX;
    verts[vert_idx++] = sprite->displayY + sprite->h;
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u0;
    texs[tex_idx++] = sprite->v1;

    // bottom-left
    verts[vert_idx++] = sprite->displayX;
    verts[vert_idx++] = sprite->displayY;
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u0;
    texs[tex_idx++] = sprite->v0;
  }

  spritelist_set_texs_and_verts_gl(nverts, vertmem, texmem);

  list->texture->bind();

  return nverts;
}

OBJECT_IMPL(BaseSpriteListRenderer, Renderable);

BaseSpriteListRenderer::BaseSpriteListRenderer(void* empty)
  : Renderable(empty), program(NULL) {
}

void BaseSpriteListRenderer::render(void* _list) {
  if(!program) {
    program = get_program(standard_program_loader);
  }

  BaseSprite list = (BaseSprite)_list;
  program->use();

  GLMemory verts;
  GLMemory texs;
  gl_bufinit(&verts);
  gl_bufinit(&texs);

  int nverts = basespritelist_set(list, &verts, &texs);

  gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));
  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE, orthographic_projection.data));
  gl_check(glDrawArrays(GL_TRIANGLES, 0, nverts));
}

int spritelist_set_texs_and_verts(BaseSprite list) {
  int nquads = list->count;
  int ntris = nquads * 2;
  int nverts = 3 * ntris;

  GLMemory vertmem;
  GLMemory texmem;
  gl_claim(gl_bufinit(&vertmem), sizeof(float) * nverts * 3);
  gl_claim(gl_bufinit(&texmem), sizeof(float) * nverts * 2);
  GLfloat* verts = (GLfloat*)vertmem.data;
  GLfloat* texs = (GLfloat*)texmem.data;

  int vert_idx = 0;
  int tex_idx = 0;

  BaseSprite _sprite;
  for(_sprite = list; _sprite != NULL;
      _sprite = _sprite->next) {
    Sprite sprite = (Sprite)_sprite;

    float sa = 0.0f;
    float ca = 1.0f;
    if(sprite->angle != 0.0f) {
      sa = sinf(sprite->angle);
      ca = cosf(sprite->angle);
    }

#define VROT_X(qx, qy) (ca * (qx) - sa * (qy))
#define VROT_Y(qx, qy) (sa * (qx) + ca * (qy))
#define SCALE_X(qx) ((qx - sprite->originX) * sprite->w)
#define SCALE_Y(qy) ((qy - sprite->originY) * sprite->h)
#define VPROCESS_X(qx, qy) (VROT_X(SCALE_X(qx), SCALE_Y(qy)) + sprite->displayX)
#define VPROCESS_Y(qx, qy) (VROT_Y(SCALE_X(qx), SCALE_Y(qy)) + sprite->displayY)

    // bottom-left
    verts[vert_idx++] = VPROCESS_X(0.0f, 0.0f);
    verts[vert_idx++] = VPROCESS_Y(0.0f, 0.0f);
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u0;
    texs[tex_idx++] = sprite->v0;

    // bottom-right
    verts[vert_idx++] = VPROCESS_X(1.0f, 0.0f);
    verts[vert_idx++] = VPROCESS_Y(1.0f, 0.0f);
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u1;
    texs[tex_idx++] = sprite->v0;

    // top-right
    verts[vert_idx++] = VPROCESS_X(1.0f, 1.0f);
    verts[vert_idx++] = VPROCESS_Y(1.0f, 1.0f);
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u1;
    texs[tex_idx++] = sprite->v1;

    // top-right
    verts[vert_idx++] = VPROCESS_X(1.0f, 1.0f);
    verts[vert_idx++] = VPROCESS_Y(1.0f, 1.0f);
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u1;
    texs[tex_idx++] = sprite->v1;

    // top-left
    verts[vert_idx++] = VPROCESS_X(0.0f, 1.0f);
    verts[vert_idx++] = VPROCESS_Y(0.0f, 1.0f);
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u0;
    texs[tex_idx++] = sprite->v1;

    // bottom-left
    verts[vert_idx++] = VPROCESS_X(0.0f, 0.0f);
    verts[vert_idx++] = VPROCESS_Y(0.0f, 0.0f);
    verts[vert_idx++] = 0.0f;
    texs[tex_idx++] = sprite->u0;
    texs[tex_idx++] = sprite->v0;
  }

  spritelist_set_texs_and_verts_gl(nverts, &vertmem, &texmem);
  return nverts;
}

OBJECT_IMPL(SpriteListRenderer, Renderable);

SpriteListRenderer::SpriteListRenderer(void *empty)
  : Renderable(empty), program(NULL) {
}

void SpriteListRenderer::render(void* _list) {
  BaseSprite list = (BaseSprite)_list;
  if(!list) return;

  if(!program) {
    program = get_program(standard_program_loader);
  }

  gl_check(program->use());

  int nverts = spritelist_set_texs_and_verts(list);
  list->texture->bind();

  gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));
  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE, orthographic_projection.data));
  gl_check(glDrawArrays(GL_TRIANGLES, 0, nverts));
}

OBJECT_IMPL(ColoredSpriteListRenderer, Renderable);

ColoredSpriteListRenderer::ColoredSpriteListRenderer(void* empty)
  : Renderable(empty), program(NULL) {
}

void ColoredSpriteListRenderer::render(void* _list) {
  BaseSprite list = (BaseSprite)_list;
  if(!list) return;

  if(!program) {
    program = get_program(standard_color_program_loader);
  }

  gl_check(program->use());

  int nverts = spritelist_set_texs_and_verts(list);

  int ncolors = nverts * 4;

  GLMemory colmem;
  GLfloat* colors = (GLfloat*)gl_claim(gl_bufinit(&colmem), sizeof(float) * ncolors)->data;

  BaseSprite _sprite;
  int color_idx = 0;
  for(_sprite = list; _sprite != NULL;
      _sprite = _sprite->next) {
    Sprite sprite = (Sprite)_sprite;

    // bottom-left
    colors[color_idx++] = sprite->color[0];
    colors[color_idx++] = sprite->color[1];
    colors[color_idx++] = sprite->color[2];
    colors[color_idx++] = sprite->color[3];

    // bottom-right
    colors[color_idx++] = sprite->color[0];
    colors[color_idx++] = sprite->color[1];
    colors[color_idx++] = sprite->color[2];
    colors[color_idx++] = sprite->color[3];

    // top-right
    colors[color_idx++] = sprite->color[0];
    colors[color_idx++] = sprite->color[1];
    colors[color_idx++] = sprite->color[2];
    colors[color_idx++] = sprite->color[3];

    // top-right
    colors[color_idx++] = sprite->color[0];
    colors[color_idx++] = sprite->color[1];
    colors[color_idx++] = sprite->color[2];
    colors[color_idx++] = sprite->color[3];

    // top-left
    colors[color_idx++] = sprite->color[0];
    colors[color_idx++] = sprite->color[1];
    colors[color_idx++] = sprite->color[2];
    colors[color_idx++] = sprite->color[3];

    // bottom-left
    colors[color_idx++] = sprite->color[0];
    colors[color_idx++] = sprite->color[1];
    colors[color_idx++] = sprite->color[2];
    colors[color_idx++] = sprite->color[3];
  }

  gl_check(glEnableVertexAttribArray(GLPARAM_OTHER0));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, colmem.buffer));
  gl_check(glVertexAttribPointer(GLPARAM_OTHER0, 4, GL_FLOAT, GL_FALSE, 0, 0));
  gl_unclaim(&colmem);

  list->texture->bind();

  gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));
  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE, orthographic_projection.data));
  gl_check(glDrawArrays(GL_TRIANGLES, 0, nverts));
}

OBJECT_IMPL(RectRenderer, Renderable);

RectRenderer::RectRenderer(void *empty)
  : Renderable(empty), program(NULL) {
}

void RectRenderer::render(void* _crect) {
  if(!program) {
    program = get_program(solid_program_loader);
  }

  ColoredRect crect = (ColoredRect)_crect;

  Rect rect = (Rect)crect;
  GLfloat rect_lines[] = {
    rect->minx, rect->miny, 0,
    rect->minx, rect->maxy, 0,

    rect->minx, rect->maxy, 0,
    rect->maxx, rect->maxy, 0,

    rect->maxx, rect->maxy, 0,
    rect->maxx, rect->miny, 0,

    rect->maxx, rect->miny, 0,
    rect->minx, rect->miny, 0
  };

  gl_check(program->use());

  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE, orthographic_projection.data));

  GLuint vertex_buffer = next_buffer();
  gl_check(glEnableVertexAttribArray(GLPARAM_VERTEX));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));
  gl_check(glBufferData(GL_ARRAY_BUFFER, sizeof(rect_lines), rect_lines, GL_DYNAMIC_DRAW));
  gl_check(glVertexAttribPointer(GLPARAM_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0));

  gl_check(glUniform4fv(program->requireUniform(UNIFORM_COLOR0), 1, crect->color));
  gl_check(glDrawArrays(GL_LINES, 0, 8));
}

OBJECT_IMPL(FilledRectRenderer, Renderable);

FilledRectRenderer::FilledRectRenderer(void *empty)
  : Renderable(empty), program(NULL) {
}

void FilledRectRenderer::render(void* _crect) {
  if(!program) {
    program = get_program(solid_program_loader);
  }

  ColoredRect crect = (ColoredRect)_crect;

  Rect rect = (Rect)crect;
  GLfloat rect_tris[] = {
    rect->minx, rect->miny, 0,
    rect->minx, rect->maxy, 0,
    rect->maxx, rect->maxy, 0,

    rect->maxx, rect->maxy, 0,
    rect->maxx, rect->miny, 0,
    rect->minx, rect->miny, 0
  };

  gl_check(program->use());
  Texture::unbind();

  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE, orthographic_projection.data));

  GLuint vertex_buffer = next_buffer();
  gl_check(glEnableVertexAttribArray(GLPARAM_VERTEX));
  gl_check(glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));
  gl_check(glBufferData(GL_ARRAY_BUFFER, sizeof(rect_tris), rect_tris, GL_DYNAMIC_DRAW));
  gl_check(glVertexAttribPointer(GLPARAM_VERTEX, 3, GL_FLOAT, GL_FALSE, 0, 0));

  gl_check(glUniform4fv(program->requireUniform(UNIFORM_COLOR0), 1, crect->color));
  gl_check(glDrawArrays(GL_TRIANGLES, 0, 6));
}

void renderable_render_to_screen(RenderableCommand* command) {
  command->renderable->render(command->args);
}

void renderables_render_to_screen(RenderableCommand* commands) {
  while(commands) {
    commands->renderable->render(commands->args);
    commands = commands->next;
  }
}
