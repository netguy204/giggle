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
#ifndef TILES_H
#define TILES_H

#include "spriteatlas.h"
#include "giggle.h"
#include "vector.h"
#include "ooc.h"
#include "mesh.h"

#include <stdio.h>
#include <stdint.h>

#include <vector>

/*
 * IT = "In Tiles"
 * IP = "In Pixels"
 */

enum TileSpecMaskEntries {
  TILESPEC_COLLIDABLE = 1,
  TILESPEC_VISIBLE = 2,
  TILESPEC_PASSABLE = 4,
};

class TileSpec {
public:
  SpriteAtlasEntry image;
  unsigned short bitmask;

  inline TileSpec()
    : image(NULL), bitmask(0) {
  }

  inline TileSpec(SpriteAtlasEntry image, int bitmask)
    : image(image), bitmask(bitmask) {
  }
};

typedef std::vector<TileSpec> TileSpecs;

struct TilePosition {
  int x, y;
};

template<>
inline void LCpush<TilePosition>(lua_State* L, TilePosition tp) {
  lua_createtable(L, 4, 0);

  lua_pushnumber(L, tp.x);
  lua_rawseti(L, -2, 1);

  lua_pushnumber(L, tp.y);
  lua_rawseti(L, -2, 2);
}

template<>
inline void LCcheck<TilePosition>(lua_State* L, TilePosition* tp, int pos) {
  if(!lua_istable(L, pos)) luaL_argerror(L, pos, "expected TilePosition table");

  lua_rawgeti(L, pos, 1);
  tp->x = luaL_checknumber(L, -1);
  lua_pop(L, 1);

  lua_rawgeti(L, pos, 2);
  tp->y = luaL_checknumber(L, -1);
  lua_pop(L, 1);
}

template<>
inline void LCpush<TileSpec>(lua_State* L, TileSpec spec) {
  lua_createtable(L, 2, 0);

  LCpush(L, spec.image);
  lua_rawseti(L, -2, 1);

  lua_pushnumber(L, spec.bitmask);
  lua_rawseti(L, -2, 2);
}

template<>
inline void LCcheck<TileSpec>(lua_State* L, TileSpec* spec, int pos) {
  if(!lua_istable(L, pos)) luaL_argerror(L, pos, "expected TileSpec table");

  lua_rawgeti(L, pos, 1);
  LCcheck(L, &spec->image, -1);
  lua_pop(L, 1);

  lua_rawgeti(L, pos, 2);
  spec->bitmask = luaL_checknumber(L, -1);
  lua_pop(L, 1);
}

class TileMap;

typedef int(*LineCallback)(TileMap* map, const TilePosition& pos, void* udata);

class TileMap : public Object {
public:
  OBJECT_PROTO(TileMap);

  TileSpecs tile_specs;
  int width_IT, height_IT;
  int tile_width_IP, tile_height_IP;
  unsigned short* tiles;

  // fake mandatory constructor
  TileMap(void* obj);

  TileMap(int width, int height, int tw, int th);
  ~TileMap();

  static TileMap* from_file(Game* game, const char* fname);

  int index(const TilePosition& pos) const;
  int validindex(const TilePosition& pos) const;
  int size() const;

  void tileposition(TilePosition& pos, int index) const;

  int nspecs() const;
  const TileSpec& get_spec(int idx) const;
  void set_spec(int idx, const TileSpec& spec);
  void add_spec(const TileSpec& spec);

  int vector_index(Vector vector) const;
  void tilecenter(Vector v, int idx) const;
  int tile_bitmask(TilePosition pos) const;
  Rect_ tile_rect(TilePosition pos) const;

  void get_walls(Walls* walls) const;

  int trace_line(const TilePosition& start, const TilePosition& end,
                 LineCallback callback, void* udata);

  BaseSprite spritelist(BaseSprite list, float x_bl, float y_bl,
                        float wpx, float hpx, Vector pos);
};

class TileMapFactory : public Object {
public:
  OBJECT_PROTO(TileMapFactory);

  TileMapFactory(void* _game);

  TileMap* from_file(const char* fname);

  Game* game;
};

typedef struct CharImage_ {
  int w, h;
  int8_t* data;
} *CharImage;

#define charimage_index(img, x, y) (((img)->w * (y)) + x)
#define charimage_set(img, x, y, val) ((img)->data[charimage_index(img, x, y)] = val)
#define charimage_get(img, x, y) ((img)->data[charimage_index(img, x, y)])

void tileposition_charimage(TilePosition& pos, CharImage img, int index);

// return true if the floodfill should include the given index
typedef int(*FloodfillCallback)(CharImage img, int index, void* udata);

int charimage_floodfill(CharImage out, CharImage input, const TilePosition& startpos,
                        int8_t value, FloodfillCallback callback, void* udata);

void charimage_init_sizeof_tilemap(CharImage img, TileMap* map);
void charimage_crosscorrelate(CharImage out, CharImage big, CharImage small);
int charimage_size(CharImage img);
void charimage_replace_value(CharImage img, int8_t from, int8_t to);

void charimage_threshold(CharImage img, int8_t min);

typedef struct LabelEntry_ {
  TilePosition pos;
  int8_t value;
} *LabelEntry;

typedef std::vector<LabelEntry_> LabelEntries;

typedef void(*LabelCallback)(LabelEntries& entries, void* udata);

void charimage_label(CharImage img, int8_t* working, LabelCallback callback, void* udata);

void charimage_write(CharImage img, FILE* target);
void charimage_spit(CharImage img, const char* filename);

void charimage_ambient_occlusion(CharImage occlusion, TileMap* map);

#endif
