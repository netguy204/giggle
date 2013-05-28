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
#ifndef TESTLIB_GL_H
#define TESTLIB_GL_H

#include "matrix.h"
#include "gl_headers.h"
#include "testlib.h"
#include "utils.h"

#include <map>

typedef enum {
  GLPARAM_VERTEX,
  GLPARAM_OTHER0,
  GLPARAM_NORMAL0,
  GLPARAM_COLOR0,
  GLPARAM_COLOR1,
  GLPARAM_FOGCOORD0,
  GLPARAM_OTHER1,
  GLPARAM_TEXCOORD0,
  GLPARAM_TEXCOORD1,
  GLPARAM_TEXCOORD2,
  GLPARAM_DONE
} ProgramParameters;

typedef enum {
  UNIFORM_MVP,
  UNIFORM_COLOR0,
  UNIFORM_TEX0,
  UNIFORM_LIGHT0_POSITION,
  UNIFORM_SCALE,
  UNIFORM_TEX_BL,
  UNIFORM_TEX_TR,
  UNIFORM_DONE
} ProgramUniforms;

extern int real_screen_width;
extern int real_screen_height;
extern float gl_version;
extern int fbo_support;

void* renderer_alloc(long size);

class Program {
 public:
  Program();
  ~Program();

  inline GLuint requireUniform(ProgramUniforms uniform) {
    if(uniforms[uniform] == -1) {
      fail_exit("uniform %d is not defined", uniform);
    }
    return uniforms[uniform];
  }

  inline void use() {
    if(program != current_program) {
      glUseProgram(program);
      current_program = program;
    }
  }

  GLuint uniforms[UNIFORM_DONE];
  GLuint program;

  static GLuint current_program;
};

typedef Program* (*ProgramLoader)(void);
typedef std::map<ProgramLoader, Program*> LoaderToProgram;
Program* get_program(ProgramLoader loader);

Program* standard_program_loader();
Program* solid_program_loader();
Program* standard_color_program_loader();
Program* tile_program_loader();

Program* renderer_link_shader(const char* vertexname, const char* fragmentname, ...);
void program_bind_uniforms(Program* p, ...);

GLuint next_buffer();

extern Matrix44 orthographic_projection;
extern GLuint tex0_uniform_location;
extern GLuint mvp_uniform_location;

class BaseSpriteListRenderer : public Renderable {
 public:
  OBJECT_PROTO(BaseSpriteListRenderer);
  BaseSpriteListRenderer(void *empty);
  virtual void render(void* args);

  Program *program;
};

class SpriteListRenderer : public Renderable {
 public:
  OBJECT_PROTO(SpriteListRenderer);
  SpriteListRenderer(void *empty);
  virtual void render(void* args);

  Program *program;
};

class ColoredSpriteListRenderer : public Renderable {
 public:
  OBJECT_PROTO(ColoredSpriteListRenderer);
  ColoredSpriteListRenderer(void *empty);
  virtual void render(void* args);

  Program *program;
};

class RectRenderer : public Renderable {
 public:
  OBJECT_PROTO(RectRenderer);
  RectRenderer(void *empty);
  virtual void render(void* args);

  Program *program;
};

class FilledRectRenderer : public Renderable {
 public:
  OBJECT_PROTO(FilledRectRenderer);
  FilledRectRenderer(void *empty);
  virtual void render(void* args);

  Program *program;
};

#define GL_CHECK_ERRORS

void gl_check_(const char * msg);

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x

#ifdef GL_CHECK_ERRORS
#define gl_check(command) command; gl_check_(__FILE__ ": " STRINGIZE(__LINE__) " " #command)
#else
#define gl_check(command) command
#endif

class GLMemory {
 public:
  GLMemory();

  void* data;
  size_t size;
  GLenum access;
  GLuint buffer;
};

GLMemory* gl_bufinit(GLMemory* mem);
GLMemory* gl_claim(GLMemory* mem, size_t sz);
void gl_unclaim(GLMemory* mem);

void swap_texture(int new_texture);
void spritelist_set_texs_and_verts_gl(int nverts, GLMemory* verts, GLMemory* texs);
int basespritelist_set(BaseSprite list, GLMemory* verts, GLMemory* texs);

#endif
