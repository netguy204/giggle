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
#include "testlib.h"
#include "vector.h"
#include "ooc.h"

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

class TilePosition {
public:
  int x, y;

  inline TilePosition() {
  }

  inline TilePosition(int x, int y)
    : x(x), y(y) {
  }
};

class TileMap;
class World;

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

  static TileMap* from_file(World* world, const char* fname);

  int index(const TilePosition& pos) const;
  int validindex(const TilePosition& pos) const;
  int size() const;

  void tileposition(TilePosition& pos, int index) const;
  int vector_index(Vector vector) const;
  void tilecenter(Vector v, int idx) const;

  int trace_line(const TilePosition& start, const TilePosition& end, LineCallback callback, void* udata);

  BaseSprite spritelist(BaseSprite list, float x_bl, float y_bl, float wpx, float hpx, Vector pos);
};

class TileMapFactory : public Object {
public:
  OBJECT_PROTO(TileMapFactory);

  TileMapFactory(void* _world);

  TileMap* from_file(const char* fname);

  World* world;
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
