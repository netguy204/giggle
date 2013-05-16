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
#ifndef GAME_H
#define GAME_H

#include "platform.h"
#include "testlib.h"
#include "spriteatlas.h"
#include "tiles.h"
#include "testlib_gl.h"
#include "pipeline.h"

class ScreenSampler : public Renderable {
 public:
  OBJECT_PROTO(ScreenSampler);
  ScreenSampler(void *empty);
  virtual ~ScreenSampler();

  virtual void render(void *args);

  GLuint fbo;
  Texture* texture;
};

class CrossfadeElement : public PipelineElement {
 public:
  CrossfadeElement(World* prev, float lifetime);
  virtual ~CrossfadeElement();

  virtual void update(long delta);

  World* prev_world;

  ScreenSampler* sampler;

  Clock clock;

  float lifetime;
  float remaining;
  int first_time;
};

class CLeftAndRight : public Component {
 public:
  OBJECT_PROTO(CLeftAndRight);

  CLeftAndRight(void* go);

  virtual void update(float dt);

  float minx, maxx;
};

class CTimer : public Component {
 public:
  OBJECT_PROTO(CTimer);

  CTimer(void* go);

  virtual void update(float dt);

  int kind;
  float time_remaining;
};

class CTestDisplay : public Component {
 public:
  OBJECT_PROTO(CTestDisplay);

  CTestDisplay(void* go);

  virtual void update(float dt);

  Vector_ offset;
  Color color;
  float w, h;
  int layer;
};

class CStaticSprite : public Component {
 public:
  OBJECT_PROTO(CStaticSprite);

  CStaticSprite(void* go);

  virtual void update(float dt);

  Vector_ offset;
  SpriteAtlasEntry entry;
  float angle;
  int layer;
};

class CColoredSprite : public CStaticSprite {
 public:
  OBJECT_PROTO(CColoredSprite);

  CColoredSprite(void* go);

  virtual void update(float dt);

  Color color;
  float w;
  float h;
};

class CSpriterSprite : public Component {
 public:
  OBJECT_PROTO(CSpriterSprite);

  CSpriterSprite(void* go);

  virtual void update(float dt);

  Animation* animation;
  Vector_ offset;
  int layer;
  float current_time;
  float time_scale;
  float scale_x;
  float scale_y;
};

enum WallpaperStyle {
  WALLPAPER_SCALE,
  WALLPAPER_TILE,
};

class CDrawWallpaper : public Component {
 public:
  OBJECT_PROTO(CDrawWallpaper);

  CDrawWallpaper(void* go);

  virtual void update(float dt);

  SpriteAtlasEntry entry;
  Vector_ offset;
  float w, h;
  int style;
  int layer;
};

class TileMapRenderer : public Renderable {
 public:
  OBJECT_PROTO(TileMapRenderer);
  TileMapRenderer(void *empty);
  virtual ~TileMapRenderer();

  virtual void render(void *args);

  Program* program;
  GLMemory verts;
  GLMemory texs;
  Texture* texture;
  int nverts;
};

class CDrawTilemap : public Component {
 public:
  OBJECT_PROTO(CDrawTilemap);

  CDrawTilemap(void* go);
  virtual ~CDrawTilemap();

  virtual void update(float dt);

  void set_map(TileMap map);
  TileMap get_map();

  TileMapRenderer *renderer;
  Vector_ offset;
  TileMap map;
  int w, h;
  int layer;
  int map_dirty;
};

class CDrawText : public Component {
 public:
  OBJECT_PROTO(CDrawText);
  CDrawText(void* go);
  virtual ~CDrawText();

  virtual void update(float dt);

  Vector_ offset;
  LString* message;
  LString* atlas;
  Color color;
  int font_size;
  int layer;
};

class CDrawConsoleText : public CDrawText {
 public:
  OBJECT_PROTO(CDrawConsoleText);
  CDrawConsoleText(void *go);

  virtual void update(float dt);

  float w;
};

struct PEntry {
  Vector_ pos;
  Vector_ vel;
  float life;
  float angle;
  float dangle;
};

enum EmitterColoring {
  COLORING_BLACKBODY,
  COLORING_BW
};

class CParticleEmitter : public Component {
 public:
  OBJECT_PROTO(CParticleEmitter);

  CParticleEmitter(void* go);
  virtual ~CParticleEmitter();
  void init();

  virtual void update(float dt);
  void init_entry(PEntry* e, float life);

  SpriteAtlasEntry entry;
  Vector_ offset;
  int nmax;
  int active;
  int layer;
  int coloring;
  float max_life;
  float max_speed;
  float max_angular_speed;
  float max_offset;
  float grav_accel;
  float start_scale;
  float end_scale;
  float start_color;
  float end_color;
  float start_alpha;
  float end_alpha;
  PEntry* entries;
};

struct GradientScreenRectParams {
  BaseSprite texture;
  Vector_ corners[4];
  Color colors[4];
};

class GradientScreenRectRenderer : public Renderable {
 public:
  OBJECT_PROTO(GradientScreenRectRenderer);
  GradientScreenRectRenderer(void* empty);
  virtual void render(void *args);

  Program* program;
};

class CGradientScreenRect : public Component {
 public:
  OBJECT_PROTO(CGradientScreenRect);

  CGradientScreenRect(void* go);
  virtual ~CGradientScreenRect();

  virtual void update(float dt);

  GradientScreenRectRenderer* renderer;

  SpriteAtlasEntry entry;
  int layer;
  Vector_ bl_p, br_p, tl_p, tr_p;
  Color bl_c, br_c, tl_c, tr_c;
};

class CTire : public Component {
 public:
  OBJECT_PROTO(CTire);

  CTire(void* go);
  virtual ~CTire();

  virtual void update(float dt);
  float drag_scale;
  float max_lateral_impulse;
};

void game_init();
void game_shutdown();

GO* platform_make(float x, float y, float w, float h);
GO* slidingplatform_make(float x, float y, float w, float h, float speed,
                         float minx, float maxx);
GO* bomb_make(Vector pos, Vector vel);

#endif
