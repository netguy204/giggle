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

Game* game;

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
OBJECT_PROPERTY(CStaticSprite, angle_offset);

CStaticSprite::CStaticSprite(void* go)
  : Component((GO*)go, PRIORITY_SHOW), entry(NULL),
    angle_offset(0.0), layer(LAYER_PLAYER) {
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
  sprite->angle = go->angle() + angle_offset;

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
  sprite->angle = go->angle() + angle_offset;
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

  gl_check(glUniform1i(program->requireUniform(UNIFORM_TEX0), 0));
  gl_check(glUniformMatrix4fv(program->requireUniform(UNIFORM_MVP),
                              1, GL_FALSE, orthographic_projection.data));
  gl_check(glDrawArrays(GL_TRIANGLES, 0, nverts));
}

OBJECT_IMPL(CDrawTilemap, Component);
OBJECT_ACCESSOR(CDrawTilemap, map, get_map, set_map);
OBJECT_ACCESSOR(CDrawTilemap, map_filename, get_map_filename, set_map_filename);
OBJECT_PROPERTY(CDrawTilemap, offset);
OBJECT_PROPERTY(CDrawTilemap, layer);

CDrawTilemap::CDrawTilemap(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), map(NULL),
    layer(LAYER_BACKGROUND), map_dirty(1) {
  vector_zero(&offset);
  renderer = new TileMapRenderer(NULL);
}

CDrawTilemap::~CDrawTilemap() {
  if(map) map->release();
  renderer->destroy();
}

TileMap* CDrawTilemap::get_map() {
  return map;
}

void CDrawTilemap::set_map(TileMap* _map) {
  if(map) map->release();
  map = _map;
  map->retain();
  map_dirty = 1;
}

const char* CDrawTilemap::get_map_filename() {
  return "<not preserved>";
}

void CDrawTilemap::set_map_filename(const char* fname) {
  if(map) map->release();
  map = TileMap::from_file(go->world->game, fname);
  map_dirty = 1;
}

void CDrawTilemap::render(Camera* camera) {
  if(!map) return;

  if(map_dirty) {
    Vector_ pos;
    go->pos(&pos);
    vector_add(&pos, &pos, &offset);

    float w = map->width_IT * map->tile_width_IP;
    float h = map->height_IT * map->tile_height_IP;

    BaseSprite sprites = map->spritelist(NULL, 0, 0, w, h, &pos);
    camera->addRenderable(layer, renderer, sprites);
    map_dirty = 0;
  } else {
    camera->addRenderable(layer, renderer, NULL);
  }
}

void game_step(long delta);

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
  : Component((GO*)_go, PRIORITY_SHOW), entry(NULL), layer(LAYER_BACKGROUND) {
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
  snprintf(buffer, sizeof(buffer), "resources/?.lua;%sengine_resources/?.lua", libbase);

  // initialize globals
  game_support_init();
  game = new Game(buffer);

  set_game_step(game_step);
}

void game_step(long delta) {
  game->update(delta);
}

void game_shutdown() {
}

void print_lstack(lua_State* L) {
  int i;
  int top = lua_gettop(L);
  for (i = 1; i <= top; i++) {  /* repeat for each level */
    int t = lua_type(L, i);
    switch (t) {

    case LUA_TSTRING:  /* strings */
      printf("`%s'", lua_tostring(L, i));
      break;

    case LUA_TBOOLEAN:  /* booleans */
      printf(lua_toboolean(L, i) ? "true" : "false");
      break;

    case LUA_TNUMBER:  /* numbers */
      printf("%g", lua_tonumber(L, i));
      break;

    default:  /* other values */
      printf("%s", lua_typename(L, t));
      break;

    }
    printf("  ");  /* put a separator */
  }
  printf("\n");  /* end the listing */
}
