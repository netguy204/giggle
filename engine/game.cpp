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
#include "game.h"
#include "vector.h"
#include "listlib.h"
#include "memlib.h"
#include "rect.h"
#include "controls.h"
#include "steering.h"
#include "tiles.h"
#include "game_ui.h"
#include "color.h"
#include "utils.h"
#include "spriter.h"

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <libgen.h>

World* world = NULL;
Universe* universe;

OBJECT_IMPL(CTimer, Component);
OBJECT_PROPERTY(CTimer, time_remaining);
OBJECT_PROPERTY(CTimer, kind);

CTimer::CTimer(void* go)
  : Component((GO*)go, PRIORITY_THINK), time_remaining(0), kind(MESSAGE_TIMER_EXPIRED) {
}

void CTimer::update(float dt) {
  this->time_remaining -= dt;

  if(this->time_remaining <= 0) {
    go->send_message(go->create_message(kind, NULL, 0));
    delete_me = 1;
  }
}

OBJECT_IMPL(CLeftAndRight, Component);
OBJECT_PROPERTY(CLeftAndRight, minx);
OBJECT_PROPERTY(CLeftAndRight, maxx);

CLeftAndRight::CLeftAndRight(void* go)
  : Component((GO*)go, PRIORITY_ACT), minx(0), maxx(0) {
}

void CLeftAndRight::update(float dt) {
  Vector_ vel, pos;
  go->pos(&pos);
  go->vel(&vel);

  if(vel.x > 0) {
    if(pos.x > this->maxx) {
      pos.x = this->maxx;
      vel.x = -vel.x;
    }
  } else {
    if(pos.x < this->minx) {
      pos.x = this->minx;
      vel.x = -vel.x;
    }
  }
}

OBJECT_IMPL(CTestDisplay, Component);
OBJECT_PROPERTY(CTestDisplay, color);
OBJECT_PROPERTY(CTestDisplay, w);
OBJECT_PROPERTY(CTestDisplay, h);
OBJECT_PROPERTY(CTestDisplay, offset);
OBJECT_PROPERTY(CTestDisplay, layer);

CTestDisplay::CTestDisplay(void* go)
  : Component((GO*)go, PRIORITY_SHOW),
    w(10), h(10), layer(LAYER_PLAYER) {
  vector_zero(&offset);
  color.r = 1.0f;
  color.g = 0.0f;
  color.b = 1.0f;
  color.a = 1.0f;
}

void CTestDisplay::render(Camera* camera) {
  Vector_ pos;
  go->pos(&pos);
  vector_add(&pos, &pos, &offset);

  ColoredRect rect = (ColoredRect)frame_alloc(sizeof(ColoredRect_));
  rect->minx = pos.x - w/2;
  rect->maxx = pos.x + w/2;
  rect->miny = pos.y - h/2;
  rect->maxy = pos.y + h/2;
  rect->color[0] = color.r;
  rect->color[1] = color.g;
  rect->color[2] = color.b;
  rect->color[3] = color.a;

  camera->addRect(&camera->testRects[layer], rect);
}

OBJECT_IMPL(CStaticSprite, Component);
OBJECT_PROPERTY(CStaticSprite, entry);
OBJECT_PROPERTY(CStaticSprite, layer);
OBJECT_PROPERTY(CStaticSprite, offset);
OBJECT_PROPERTY(CStaticSprite, angle);

CStaticSprite::CStaticSprite(void* go)
  : Component((GO*)go, PRIORITY_SHOW), entry(NULL),
    layer(LAYER_PLAYER), angle(0.0) {
  vector_zero(&offset);
}

void CStaticSprite::render(Camera* camera) {
  Vector_ pos;
  go->pos(&pos);
  vector_add(&pos, &pos, &offset);

  Sprite sprite = frame_make_sprite();
  sprite_fillfromentry(sprite, entry);
  sprite->displayX = pos.x;
  sprite->displayY = pos.y;
  sprite->originX = 0.5;
  sprite->originY = 0.5;
  sprite->angle = angle;

  camera->addSprite(&camera->layers[layer], sprite);
}

OBJECT_IMPL(CColoredSprite, CStaticSprite);
OBJECT_PROPERTY(CColoredSprite, color);
OBJECT_PROPERTY(CColoredSprite, w);
OBJECT_PROPERTY(CColoredSprite, h);

CColoredSprite::CColoredSprite(void* _go)
  : CStaticSprite(_go) {
  color.r = 1.0;
  color.g = 1.0;
  color.b = 1.0;
  color.a = 1.0;
}

void CColoredSprite::render(Camera* camera) {
  Vector_ pos;
  go->pos(&pos);
  vector_add(&pos, &pos, &offset);

  Sprite sprite = frame_make_sprite();
  sprite_fillfromentry(sprite, entry);
  sprite->displayX = pos.x;
  sprite->displayY = pos.y;
  sprite->originX = 0.5;
  sprite->originY = 0.5;
  sprite->angle = angle;
  sprite->color[0] = color.r;
  sprite->color[1] = color.g;
  sprite->color[2] = color.b;
  sprite->color[3] = color.a;
  sprite->w = w;
  sprite->h = h;

  camera->addSprite(&camera->particles[layer], sprite);
}

OBJECT_IMPL(CSpriterSprite, Component);
OBJECT_PROPERTY(CSpriterSprite, animation);
OBJECT_PROPERTY(CSpriterSprite, offset);
OBJECT_PROPERTY(CSpriterSprite, layer);
OBJECT_PROPERTY(CSpriterSprite, current_time);
OBJECT_PROPERTY(CSpriterSprite, time_scale);
OBJECT_PROPERTY(CSpriterSprite, scale_x);
OBJECT_PROPERTY(CSpriterSprite, scale_y);

CSpriterSprite::CSpriterSprite(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), animation(NULL), layer(LAYER_BACKGROUND),
    current_time(0), time_scale(1), scale_x(1), scale_y(1) {
  vector_zero(&offset);
}

void CSpriterSprite::update(float dt) {
  current_time += dt * time_scale;
}

void CSpriterSprite::render(Camera* camera) {
  if(!animation) return;

  Vector_ pos;
  go->pos(&pos);

  vector_add(&pos, &pos, &offset);

  BaseSprite list = camera->layers[layer];
  list = spriter_append(list, animation, &pos, scale_x, scale_y, current_time * 1000);
  camera->layers[layer] = list;
}

OBJECT_IMPL(CDrawWallpaper, Component);
OBJECT_PROPERTY(CDrawWallpaper, entry);
OBJECT_PROPERTY(CDrawWallpaper, offset);
OBJECT_PROPERTY(CDrawWallpaper, w);
OBJECT_PROPERTY(CDrawWallpaper, h);
OBJECT_PROPERTY(CDrawWallpaper, style);
OBJECT_PROPERTY(CDrawWallpaper, layer);

CDrawWallpaper::CDrawWallpaper(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), style(WALLPAPER_TILE), layer(LAYER_BACKGROUND) {
  vector_zero(&offset);
}

void CDrawWallpaper::render(Camera* camera) {
  Vector_ pos;
  go->pos(&pos);

  vector_add(&pos, &pos, &offset);

  float x_bl = (pos.x - w/2);
  if(x_bl > screen_width) return;

  float y_bl = (pos.y - h/2);
  if(y_bl > screen_height) return;

  float x_tr = (pos.x + w/2);
  if(x_tr < 0) return;

  float y_tr = (pos.y + h/2);
  if(y_tr < 0) return;

  // now we have some overlap, clamp the tr to the screen and figure
  // out the bl offset
  x_tr = MIN(x_tr, screen_width);
  y_tr = MIN(y_tr, screen_height);

  if(x_bl < 0) {
    x_bl += floorf(fabs(x_bl) / entry->w) * entry->w;
  }

  if(y_bl < 0) {
    y_bl += floorf(fabs(y_bl) / entry->h) * entry->h;
  }

  if(style != WALLPAPER_TILE) {
    fail_exit("i haven't bothered to implement the other wallpaper styles yet\n");
  }

  float y = y_bl;
  while(y < y_tr) {
    float x = x_bl;
    while(x < x_tr) {
      BaseSprite sprite = (BaseSprite)frame_alloc(sizeof(BaseSprite_));
      sprite_fillfromentry(sprite, entry);
      sprite->displayX = x;
      sprite->displayY = y;
      camera->addSprite(&camera->baseLayers[layer], sprite);
      x += entry->w;
    }
    y += entry->h;
  }
}

void LCcheck_spec(lua_State* L, int pos, TileSpec spec) {
  if(!lua_istable(L, pos)) {
    luaL_error(L, "position %d does not contain a spec-table", pos);
  }

  lua_getfield(L, pos, "image");
  LCcheck(L, &spec->image, -1);
  lua_pop(L, 1);

  lua_getfield(L, pos, "bitmask");
  if(lua_isnil(L, -1)) {
    spec->bitmask = TILESPEC_VISIBLE;
  } else {
    spec->bitmask = lua_tointeger(L, -1);
  }
  lua_pop(L, 1);
}

int LCtable_count(lua_State* L, int pos) {
  // no reliable way to get the size of an array table so we just keep
  // checking indicies till we get null
  int index = 1;
  lua_rawgeti(L, pos, index);
  while(!lua_isnil(L, -1)) {
    lua_pop(L, 1);
    index++;
    lua_rawgeti(L, pos, index);
  }
  lua_pop(L, 1);
  return index - 1;
}

#define TM_GET_NUMBER(var, field)                                       \
  lua_getfield(L, pos, field);                                          \
  if(!lua_isnumber(L, -1)) {                                            \
    luaL_error(L, "map-table must contain a `%s' field", field);        \
  }                                                                     \
  var = lua_tointeger(L, -1);                                           \
  lua_pop(L, 1);

template<>
void PropertyTypeImpl<TileMap>::LCset_value(Object* obj, lua_State* L, int pos) const {
  if(!lua_istable(L, pos)) {
    luaL_error(L, "position %d does not contain a map-table", pos);
  }

  int width, height, nSpecs, tWidth, tHeight;
  TM_GET_NUMBER(width, "width");
  TM_GET_NUMBER(height, "height");
  TM_GET_NUMBER(tWidth, "tile_width");
  TM_GET_NUMBER(tHeight, "tile_height");

  lua_getfield(L, pos, "specs");
  if(!lua_istable(L, -1)) {
    luaL_error(L, "map-table must contain a `specs' key (pos %d)", pos);
  }

  // we automatically create the nil spec entry
  nSpecs = LCtable_count(L, -1) + 1;
  TileMap map = tilemap_make(width, height, nSpecs, tWidth, tHeight);
  map->tile_specs[0].image = NULL;
  map->tile_specs[0].bitmask = 0;

  // fill in the user defined specs
  for(int speci = 1; speci < nSpecs; ++speci) {
    lua_rawgeti(L, -1, speci);
    LCcheck_spec(L, -1, &map->tile_specs[speci]);
    lua_pop(L, 1);
  }
  lua_pop(L, 1); // specs

  // fill in the tiles
  lua_getfield(L, pos, "tiles");
  const int nTiles = width * height;
  for(int ii = 1; ii <= nTiles; ++ii) {
    lua_rawgeti(L, -1, ii);
    if(!lua_isnumber(L, -1)) {
      luaL_error(L, "all values in `tiles' array must be integers");
    }
    map->tiles[ii - 1] = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }
  lua_pop(L, 1); // tiles

  set_value(obj, &map);
}

OBJECT_IMPL(TileMapRenderer, Renderable);

TileMapRenderer::TileMapRenderer(void *empty)
  : Renderable(empty), program(NULL) {
}

TileMapRenderer::~TileMapRenderer() {
  if(verts.buffer) {
    glDeleteBuffers(1, &verts.buffer);
    glDeleteBuffers(1, &texs.buffer);
  }
}

void TileMapRenderer::render(void* _sprites) {
  BaseSprite sprites = (BaseSprite)_sprites;

  if(!verts.buffer) {
    glGenBuffers(1, &verts.buffer);
    glGenBuffers(1, &texs.buffer);
  }

  if(!program) {
    program = get_program(standard_program_loader);
  }

  gl_check(program->use());

  if(sprites) {
    nverts = basespritelist_set(sprites, &verts, &texs);
    texture = sprites->texture;
  } else {
    spritelist_set_texs_and_verts_gl(nverts, &verts, &texs);
    texture->bind();
  }

  GLfloat light[] = {0.0, 1000.0, 1000.0};
  gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));
  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE, orthographic_projection.data));
  gl_check(glDrawArrays(GL_TRIANGLES, 0, nverts));
}

OBJECT_IMPL(CDrawTilemap, Component);
OBJECT_ACCESSOR(CDrawTilemap, map, get_map, set_map);
OBJECT_PROPERTY(CDrawTilemap, offset);
OBJECT_PROPERTY(CDrawTilemap, w);
OBJECT_PROPERTY(CDrawTilemap, h);
OBJECT_PROPERTY(CDrawTilemap, layer);

CDrawTilemap::CDrawTilemap(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), map(NULL), layer(LAYER_BACKGROUND),
    w(300), h(300), map_dirty(1) {
  vector_zero(&offset);
  renderer = new TileMapRenderer(NULL);
}

CDrawTilemap::~CDrawTilemap() {
  if(map) tilemap_free(map);
  renderer->destroy();
}

TileMap CDrawTilemap::get_map() {
  return map;
}

void CDrawTilemap::set_map(TileMap _map) {
  if(map) tilemap_free(map);
  map = _map;
  map_dirty = 1;
}

void CDrawTilemap::render(Camera* camera) {
  if(!map) return;

  if(map_dirty) {
    Vector_ pos;
    go->pos(&pos);
    vector_add(&pos, &pos, &offset);
    map->x_bl = pos.x;
    map->y_bl = pos.y;

    BaseSprite sprites = tilemap_spritelist(NULL, map, 0, 0, w, h);
    camera->addRenderable(layer, renderer, sprites);
    map_dirty = 0;
  } else {
    camera->addRenderable(layer, renderer, NULL);
  }
}

OBJECT_IMPL(CDrawText, Component);
OBJECT_PROPERTY(CDrawText, message);
OBJECT_PROPERTY(CDrawText, atlas);
OBJECT_PROPERTY(CDrawText, font_size);
OBJECT_PROPERTY(CDrawText, layer);
OBJECT_PROPERTY(CDrawText, color);
OBJECT_PROPERTY(CDrawText, offset);

CDrawText::CDrawText(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), layer(LAYER_MENU), font_size(FONT_MEDIUM),
    message(NULL), atlas(NULL) {
  vector_zero(&offset);
  color.r = 1.0;
  color.g = 1.0;
  color.b = 1.0;
  color.a = 1.0;
}

CDrawText::~CDrawText() {
  if(message) free_lstring(message);
  if(atlas) free_lstring(atlas);
}

void CDrawText::render(Camera* camera) {
  if(!message) return;
  if(!atlas) {
    fail_exit("CDrawText was not given a font atlas");
  }

  BaseSprite* spritep = &camera->particles[layer];
  SpriteAtlas atlasobj = go->world->atlas(atlas->str);
  Vector_ pos;
  go->pos(&pos);

  vector_add(&pos, &pos, &offset);

  *spritep = spritelist_from_string(*spritep, atlasobj, (FontSize)font_size,
                                    message->str,
                                    pos.x, pos.y, &color);
}

OBJECT_IMPL(CDrawConsoleText, CDrawText);
OBJECT_PROPERTY(CDrawConsoleText, w);

CDrawConsoleText::CDrawConsoleText(void *_go)
  : CDrawText(_go) {
  w = 100;
}

void CDrawConsoleText::render(Camera* camera) {
  if(!message) return;
  if(!atlas) {
    fail_exit("CDrawConsoleText was not given a font atlas");
  }

  BaseSprite* spritep = &camera->particles[layer];
  SpriteAtlas atlasobj = go->world->atlas(atlas->str);
  Vector_ pos;
  go->pos(&pos);

  vector_add(&pos, &pos, &offset);

  *spritep = spritelist_from_consoletext(
        *spritep, atlasobj, message->str, pos.x, pos.y,
        w, &color);
}

OBJECT_IMPL(ScreenSampler, Renderable);

ScreenSampler::ScreenSampler(void *empty)
  : Renderable(empty), fbo(0), texture(new Texture()) {
}

ScreenSampler::~ScreenSampler() {
  if(fbo) {
    glDeleteFramebuffers(1, &fbo);
    delete texture;
  }
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

void ScreenSampler::render(void *args) {
  // make sure FBOs are supported
  if(!fbo_support) return;

  if(fbo == 0) {
    // initialize memory
    gl_check(glGenTextures(1, &texture->texid));
    gl_check(glGenFramebuffers(1, &fbo));
    gl_check(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

    texture->bind();
    gl_check(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                          real_screen_width, real_screen_height,
                          0, GL_RGB, GL_UNSIGNED_BYTE, NULL));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    gl_check(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));

    gl_check(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->texid, 0));

    //gl_check(glDrawBuffer(GL_COLOR_ATTACHMENT0));

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE) {
      fail_exit("frame buffer is not complete: %s", framebufferstatus(status));
    }
    gl_check(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  }

  // if args is null then we're setting up, if not then we're tearing
  // down
  if(!args) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

void game_step(long delta);

int world_reset_requested = 0;

int Lreset_world(lua_State* L) {
  world_reset_requested = 1;
  return 0;
}

int Lrandom_gaussian(lua_State* L) {
  lua_pushnumber(L, random_next_gaussian(&rgen));
  return 1;
}

int Lset_transform(lua_State* L) {
  float xmin = luaL_checknumber(L, 1);
  float xmax = luaL_checknumber(L, 2);
  float ymin = luaL_checknumber(L, 3);
  float ymax = luaL_checknumber(L, 4);
  matrix_orthographic_proj(&orthographic_projection,
                           xmin, xmax,
                           ymin, ymax,
                           -1.0f, 1.0f);
  return 0;
}

void init_world() {
  LOGI("init_world");
  World* old_world = world;

  world = new World(universe);

  lua_register(world->L, "reset_world", Lreset_world);
  lua_register(world->L, "random_gaussian", Lrandom_gaussian);
  lua_register(world->L, "set_transform", Lset_transform);

  LCpush(world->L, universe);
  lua_setglobal(world->L, "universe");

  world->load_level("resources/init.lua");

  world_reset_requested = 0;
  LOGI("init complete");
}

void game_support_init() {
  color_init();
}

OBJECT_IMPL(GradientScreenRectRenderer, Renderable);

GradientScreenRectRenderer::GradientScreenRectRenderer(void* empty)
  : Renderable(empty), program(NULL) {
}

void GradientScreenRectRenderer::render(void *args) {
  if(!program) {
    program = get_program(standard_color_program_loader);
  }

  GradientScreenRectParams* params = (GradientScreenRectParams*)args;

  GLfloat verts[] = {
    params->corners[0].x, params->corners[0].y, 0.0f, // bl
    params->corners[1].x, params->corners[1].y, 0.0f, // br
    params->corners[2].x, params->corners[2].y, 0.0f, // tr
    params->corners[2].x, params->corners[2].y, 0.0f, // tr
    params->corners[3].x, params->corners[3].y, 0.0f, // tl
    params->corners[0].x, params->corners[0].y, 0.0f  // bl
  };

  GLfloat colors[] = {
    params->colors[0].r, params->colors[0].g, params->colors[0].b, params->colors[0].a,
    params->colors[1].r, params->colors[1].g, params->colors[1].b, params->colors[1].a,
    params->colors[2].r, params->colors[2].g, params->colors[2].b, params->colors[2].a,
    params->colors[2].r, params->colors[2].g, params->colors[2].b, params->colors[2].a,
    params->colors[3].r, params->colors[3].g, params->colors[3].b, params->colors[3].a,
    params->colors[0].r, params->colors[0].g, params->colors[0].b, params->colors[0].a,
  };

  GLfloat texs[] = {
    params->texture->u0, params->texture->v0,
    params->texture->u1, params->texture->v0,
    params->texture->u1, params->texture->v1,
    params->texture->u1, params->texture->v1,
    params->texture->u0, params->texture->v1,
    params->texture->u0, params->texture->v0
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
  params->texture->texture->bind();

  gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));
  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE, orthographic_projection.data));
  gl_check(glDrawArrays(GL_TRIANGLES, 0, 6));
}

OBJECT_IMPL(CGradientScreenRect, Component);
OBJECT_PROPERTY(CGradientScreenRect, entry);
OBJECT_PROPERTY(CGradientScreenRect, layer);
OBJECT_PROPERTY(CGradientScreenRect, bl_p);
OBJECT_PROPERTY(CGradientScreenRect, br_p);
OBJECT_PROPERTY(CGradientScreenRect, tl_p);
OBJECT_PROPERTY(CGradientScreenRect, tr_p);
OBJECT_PROPERTY(CGradientScreenRect, bl_c);
OBJECT_PROPERTY(CGradientScreenRect, br_c);
OBJECT_PROPERTY(CGradientScreenRect, tl_c);
OBJECT_PROPERTY(CGradientScreenRect, tr_c);

CGradientScreenRect::CGradientScreenRect(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), layer(LAYER_BACKGROUND), entry(NULL) {
  renderer = new GradientScreenRectRenderer(NULL);
}

CGradientScreenRect::~CGradientScreenRect() {
  renderer->destroy();
}

void CGradientScreenRect::render(Camera* camera) {
  GradientScreenRectParams* params
    = (GradientScreenRectParams*)frame_alloc(sizeof(GradientScreenRectParams));
  params->corners[0] = bl_p;
  params->corners[1] = br_p;
  params->corners[2] = tr_p;
  params->corners[3] = tl_p;

  params->colors[0] = bl_c;
  params->colors[1] = br_c;
  params->colors[2] = tr_c;
  params->colors[3] = tl_c;
  params->texture = frame_make_sprite();
  sprite_fillfromentry(params->texture, entry);

  camera->addRenderable(layer, renderer, params);
}

OBJECT_IMPL(CTire, Component);
OBJECT_PROPERTY(CTire, drag_scale);
OBJECT_PROPERTY(CTire, max_lateral_impulse);

CTire::CTire(void* _go)
  : Component((GO*)_go, PRIORITY_THINK), drag_scale(.1),
    max_lateral_impulse(.1) {
  go->body->SetType(b2_dynamicBody);
}

CTire::~CTire() {
}

void CTire::update(float dt) {
  // get the lateral velocity
  b2Vec2 latHat = go->body->GetWorldVector(b2Vec2(1,0));
  b2Vec2 latVel = b2Dot(latHat, go->body->GetLinearVelocity()) * latHat;

  // forward velocity
  b2Vec2 forwardHat = go->body->GetWorldVector(b2Vec2(0,1));
  b2Vec2 forwardVel = b2Dot(forwardHat, go->body->GetLinearVelocity()) * forwardHat;

  // compute corrective impulse
  b2Vec2 impulse = go->body->GetMass() * -latVel;
  if(impulse.Length() > max_lateral_impulse) {
    impulse *= max_lateral_impulse / impulse.Length();
  }
  go->body->ApplyLinearImpulse(impulse, go->body->GetWorldCenter());

  // apply drag force
  go->body->ApplyForce(-drag_scale * forwardVel, go->body->GetWorldCenter());

}

void game_init(int argc, char** argv) {
  // build up the default lua path
  char buffer[1024];
  char* mydir = dirname(strdup(argv[0]));
  snprintf(buffer, sizeof(buffer), "%s/../resources/?.lua;%s/../engine_resources/?.lua", mydir, mydir);
  free(mydir);

  // initialize globals
  game_support_init();
  universe = new Universe(buffer);

  init_world();
  set_game_step(game_step);
}

void game_step(long delta) {
  if(world_reset_requested) {
    init_world();
  }

  world->update(delta);
  universe->update(delta);
}

void game_shutdown() {
}

void print_lstack() {
  lua_State* L = world->L;
  for(int ii = 0; ii < lua_gettop(L); ++ii) {
    printf("pos %d type %d\n", ii, lua_type(L, ii));
  }
}
