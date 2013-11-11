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
#include "tiles.h"
#include "heapvector.h"
#include "utils.h"
#include "filenumbers.h"
#include "gameobject.h"

#include <math.h>
#include <stdio.h>
#include <assert.h>

#include <vector>


OBJECT_IMPL(TileMap, Object);
OBJECT_PROPERTY(TileMap, width_IT);
OBJECT_PROPERTY(TileMap, height_IT);
OBJECT_PROPERTY(TileMap, tile_width_IP);
OBJECT_PROPERTY(TileMap, tile_height_IP);

TileMap::TileMap(void* empty) {
  fail_exit("invalid constructor");
}

TileMap::TileMap(int width, int height, int tw, int th)
  : width_IT(width), height_IT(height), tile_width_IP(tw), tile_height_IP(th) {
  int num_tiles = width * height;
  tiles = (unsigned short*)malloc(num_tiles * sizeof(unsigned short));
}

TileMap::~TileMap() {
  free(tiles);
}

TileMap* TileMap::from_file(Game* game, const char* fname) {
  FILE* f = fopen(fname, "r");
  if(!f) fail_exit("couldn't open %s", fname);

  unsigned short w, h, tw, th, nspecs;
  read_ushort(f, &w);
  read_ushort(f, &h);
  read_ushort(f, &tw);
  read_ushort(f, &th);

  TileMap* map = new TileMap(w, h, tw, th);

  // read and build the specs
  read_ushort(f, &nspecs);
  for(unsigned ii = 0; ii < nspecs; ++ii) {
    TileSpec spec;
    char atlas[32];
    char entry[32];

    read_fstring(f, atlas);
    read_fstring(f, entry);
    read_ushort(f, &spec.bitmask);

    if(strlen(atlas) == 0) {
      // handle the "nothing image" case
      spec.image = NULL;
    } else {
      spec.image = game->atlas_entry(atlas, entry);
    }

    map->tile_specs.push_back(spec);
  }

  // read the map data
  read_ushorts(f, map->tiles, w * h);

  fclose(f);

  return map;
}

int TileMap::index(const TilePosition& pos) const {
  return width_IT * pos.y + pos.x;
}

int TileMap::validindex(const TilePosition& pos) const {
  return pos.x >= 0 && pos.x < width_IT
    && pos.y >= 0 && pos.y < height_IT;
}

int TileMap::size() const {
  return width_IT * height_IT;
}

void TileMap::tileposition(TilePosition& pos, int index) const {
  pos.x = index % width_IT;
  pos.y = index / width_IT;
}

int TileMap::nspecs() const {
  return tile_specs.size();
}
OBJECT_METHOD(TileMap, nspecs, int, ());

const TileSpec& TileMap::get_spec(int idx) const {
  return tile_specs[idx];
}
OBJECT_METHOD(TileMap, get_spec, TileSpec, (int));

void TileMap::set_spec(int idx, const TileSpec& spec) {
  tile_specs[idx] = spec;
}
OBJECT_METHOD(TileMap, set_spec, void, (int, TileSpec));

void TileMap::add_spec(const TileSpec& spec) {
  tile_specs.push_back(spec);
}
OBJECT_METHOD(TileMap, add_spec, void, (TileSpec));

int TileMap::vector_index(Vector vector) const {
  float x = vector->x / tile_width_IP;
  float y = vector->y / tile_height_IP;

  return (int)floorf(x) + (int)floorf(y) * width_IT;
}

void TileMap::tilecenter(Vector v, int idx) const {
  TilePosition pos;
  tileposition(pos, idx);

  v->x = tile_width_IP * pos.x + tile_width_IP / 2;
  v->y = tile_height_IP * pos.y + tile_height_IP / 2;
}

int TileMap::tile_bitmask(TilePosition pos) const {
  int idx = index(pos);
  unsigned short tile = tiles[idx];
  const TileSpec* spec = &tile_specs[tile];
  return spec->bitmask;
}
OBJECT_METHOD(TileMap, tile_bitmask, int, (TilePosition));

Rect_ TileMap::tile_rect(TilePosition pos) const {
  Vector_ center;
  Rect_ result;

  tilecenter(&center, index(pos));
  rect_centered(&result, &center, tile_width_IP, tile_height_IP);
  return result;
}
OBJECT_METHOD(TileMap, tile_rect, Rect_, (TilePosition));

void TileMap::get_walls(Walls* walls) const {
  Vector_ bl = {0, 0};
  Vector_ br = {(float)(width_IT * tile_width_IP), 0};
  Vector_ tl = {0, (float)(height_IT * tile_height_IP)};
  Vector_ tr = {(float)(width_IT * tile_width_IP),
                (float)(height_IT * tile_height_IP)};

  // find all of the horizontal walls, starting with the map bounds
  walls->add_wall(tl, tr);
  walls->add_wall(bl, br);

  for(int ii = 0; ii < (height_IT-1); ++ii) {
    int start_idx = -1;
    int nii = ii + 1;

    for(int jj = 0; jj < width_IT; ++jj) {
      TilePosition pos1;
      pos1.x = jj;
      pos1.y = ii;

      TilePosition pos2;
      pos2.x = jj;
      pos2.y = nii;

      if((tile_bitmask(pos1) & TILESPEC_COLLIDABLE) !=
         (tile_bitmask(pos2) & TILESPEC_COLLIDABLE)) {
        // there is a boundary between our row and the next
        if(start_idx == -1) {
          // and this is the start of a new wall
          start_idx = jj;
        }
      } else {
        // there is not a boundary
        if(start_idx != -1) {
          // and that was the end of a wall
          float x1 = start_idx * tile_width_IP;
          float x2 = jj * tile_width_IP;
          float nudge = 0.5;
          float y = nii * tile_height_IP;
          Vector_ v1 = {x1, y};
          Vector_ v2 = {x2, y};
          walls->add_wall(v1, v2);
          start_idx = -1;
        }
      }
    }
  }

  // now add all the vertical walls, starting with the map bounds
  walls->add_wall(bl, tl);
  walls->add_wall(br, tr);

  for(int jj = 0; jj < (width_IT-1); ++jj) {
    int start_idx = -1;
    int njj = jj + 1;

    for(int ii = 0; ii < height_IT; ++ii) {
      TilePosition pos1;
      pos1.x = jj;
      pos1.y = ii;
      TilePosition pos2;
      pos2.x = njj;
      pos2.y = ii;

      if((tile_bitmask(pos1) & TILESPEC_COLLIDABLE) !=
         (tile_bitmask(pos2) & TILESPEC_COLLIDABLE)) {
        // there is a boundary between our column and the next
        if(start_idx == -1) {
          // and this is the start of a new wall
          start_idx = ii;
        }
      } else {
        // there is not a boundary
        if(start_idx != -1) {
          // and that was the end of a wall
          float x = njj * tile_width_IP;
          float y1 = start_idx * tile_height_IP;
          float y2 = ii * tile_height_IP;
          Vector_ v1 = {x, y1};
          Vector_ v2 = {x, y2};
          walls->add_wall(v1, v2);
          start_idx = -1;
        }
      }
    }
  }
}
OBJECT_METHOD(TileMap, get_walls, void, (Walls*));

int TileMap::trace_line(const TilePosition& start, const TilePosition& end,
                        LineCallback callback, void* udata) {
  /* Bresenham's algorithm */
  TilePosition pos = start;
  int dx = abs(start.x - end.x);
  int dy = abs(start.y - end.y);

  int sx, sy;
  if(start.x < end.x) {
    sx = 1;
  } else {
    sx = -1;
  }

  if(start.y < end.y) {
    sy = 1;
  } else {
    sy = -1;
  }

  int err = dx - dy;
  while(1) {
    int result = callback(this, pos, udata);
    if(result) return result;

    if (pos.x == end.x && pos.y == end.y) return 0;
    int e2 = 2 * err;
    if (e2 > -dy) {
      err = err - dy;
      pos.x = pos.x + sx;
    }
    if (e2 < dx) {
      err = err + dx;
      pos.y =  pos.y + sy;
    }
  }
}


int clamp(int val, int min, int max) {
  if(val < min) return min;
  if(val > max) return max;
  return val;
}

BaseSprite TileMap::spritelist(BaseSprite spritelist, float x_bl, float y_bl,
                               float wpx, float hpx, Vector pos) {
  float mx_bl = x_bl - pos->x;
  float my_bl = y_bl - pos->y;
  float mx_tr = mx_bl + wpx;
  float my_tr = my_bl + hpx;

  int tx_bl = clamp(floor(mx_bl / tile_width_IP), 0, width_IT);
  int ty_bl = clamp(floor(my_bl / tile_height_IP), 0, height_IT);
  int tx_tr = clamp(ceil(mx_tr / tile_width_IP), 0, width_IT);
  int ty_tr = clamp(ceil(my_tr / tile_height_IP), 0, height_IT);

  int ox = (int)floorf((pos->x + tx_bl * tile_width_IP) - x_bl);
  int oy = (int)floorf((pos->y + ty_bl * tile_height_IP) - y_bl);

  int xx, yy;
  for(yy = 0; yy < ty_tr - ty_bl; ++yy) {
    int yoffset = width_IT * (ty_bl + yy);
    int y = oy + (tile_height_IP * yy);

    for(xx = 0; xx < tx_tr - tx_bl; ++xx) {
      int offset = yoffset + tx_bl + xx;
      int x = ox + (tile_width_IP * xx);

      int tile = tiles[offset];
      TileSpec* spec = &tile_specs[tile];
      if((spec->bitmask & TILESPEC_VISIBLE) == 0) continue;

      BaseSprite sprite = (BaseSprite)GIGGLE->renderer->alloc(sizeof(BaseSprite_));
      sprite_fillfromentry(sprite, spec->image);
      sprite->displayX = x;
      sprite->displayY = y;
      sprite_append(spritelist, sprite);
    }
  }

  return spritelist;
}

OBJECT_IMPL(TileMapFactory, Object);

TileMapFactory::TileMapFactory(void* _game)
  : game((Game*)_game) {
}

TileMap* TileMapFactory::from_file(const char* fname) {
  TileMap* tm = TileMap::from_file(game, fname);
  tm->reference_count = 0; // disown
  return tm;
}
OBJECT_METHOD(TileMapFactory, from_file, TileMap*, (const char*));

void tileposition_charimage(TilePosition& pos, CharImage img, int index) {
  pos.x = index % img->w;
  pos.y = index / img->w;
}

int charimage_floodfill(CharImage out, CharImage input, const TilePosition& startpos,
                        int8_t value, FloodfillCallback callback, void* udata) {
  int8_t* memory = out->data;
  assert(charimage_size(input) == charimage_size(out));

  int max_index = charimage_size(input);
  int start = charimage_index(input, startpos.x, startpos.y);
  int kind = input->data[start];
  int row = input->w;
  std::vector<int> stack;

  if(callback) callback(input, start, udata);
  memory[start] = value;
  stack.push_back(start);
  int count = 0;

#define CAN_VISIT(pos) (((callback && callback(input, pos, udata)) || input->data[pos] == kind) \
                        && memory[pos] != value)

  while(!stack.empty()) {
    count += 1;

    int start = stack.back(); stack.pop_back();

    int above = start + row;
    if(above < max_index && CAN_VISIT(above)) {
      memory[above] = value;
      stack.push_back(above);
    }

    int below = start - row;
    if(below >= 0 && CAN_VISIT(below)) {
      memory[below] = value;
      stack.push_back(below);
    }

    int left = start - 1;
    if(left >= 0 && CAN_VISIT(left)) {
      memory[left] = value;
      stack.push_back(left);
    }

    int right = start + 1;
    if(right < max_index && CAN_VISIT(right)) {
      memory[right] = value;
      stack.push_back(right);
    }
  }

  return count;
}

void charimage_init_sizeof_tilemap(CharImage img, TileMap* map) {
  img->w = map->width_IT;
  img->h = map->height_IT;
  img->data = (int8_t*)malloc(img->w * img->h);
}

void charimage_crosscorrelate(CharImage out, CharImage big, CharImage small) {
  int out_width = big->w - small->w;
  int out_height = big->h - small->h;

  assert(out->w >= out_width);
  assert(out->h >= out_height);

  int ox, oy, sx, sy;
  for(oy = 0; oy < out_height; ++oy) {
    for(ox = 0; ox < out_width; ++ox) {
      charimage_set(out, ox, oy, 0);

      for(sy = 0; sy < small->h; ++sy) {
        for(sx = 0; sx < small->w; ++sx) {
          int8_t prod = charimage_get(big, ox + sx, oy + sy)
            * charimage_get(small, sx, sy);
          out->data[charimage_index(out, ox, oy)] += prod;
        }
      }
    }
  }
}

int charimage_size(CharImage img) {
  return img->w * img->h;
}

void charimage_replace_value(CharImage img, int8_t from, int8_t to) {
  int xx;
  for(xx = 0; xx < charimage_size(img); ++xx) {
    if(img->data[xx] == from) {
      img->data[xx] = to;
    }
  }
}

void charimage_threshold(CharImage img, int8_t min) {
  int size = charimage_size(img);
  int ii;
  for(ii = 0; ii < size; ++ii) {
    if(img->data[ii] < min) {
      img->data[ii] = 0;
    }
  }
}


int label_floodfill_callback(CharImage img, int index, void* udata) {
  int8_t value = img->data[index];
  if(value == 0) return 0;

  LabelEntries* hv = (LabelEntries*)udata;

  struct LabelEntry_ entry;
  tileposition_charimage(entry.pos, img, index);
  entry.value = value;
  hv->push_back(entry);
  return 1;
}

void charimage_label(CharImage img, int8_t* working, LabelCallback callback, void* udata) {
  int size = charimage_size(img);
  memset(working, 0, size);

  LabelEntries hv;
  struct CharImage_ out = { img->w, img->h, working };

  int ii;
  for(ii = 0; ii < size; ++ii) {
    // if this is a new region
    if(img->data[ii] != 0 && working[ii] == 0) {
      hv.clear();

      TilePosition pos;
      tileposition_charimage(pos, img, ii);

      charimage_floodfill(&out, img, pos, 1, label_floodfill_callback, &hv);

      callback(hv, udata);
    }
  }
}

void charimage_write(CharImage img, FILE* target) {
  int ii, jj;
  for(jj = 0; jj < img->h; ++jj) {
    for(ii = 0; ii < img->w; ++ii) {
      if(ii > 0) {
        fprintf(target, ", ");
      }
      fprintf(target, "%d", (int)charimage_get(img, ii, jj));
    }
    fprintf(target, "\n");
  }
}

void charimage_spit(CharImage img, const char* filename) {
  FILE* target = fopen(filename, "w");
  if(target == NULL) {
    fail_exit("charimage_spit: couldn't open %s", filename);
  }
  charimage_write(img, target);
  fclose(target);
}

static int region[][3] = {
  {0, 1, 2},
  {1, 1, 3},
  {1, 0, 2},
  {1, -1, 3},
  {0, -1, 2},
  {-1, -1, 3},
  {-1, 0, 2},
  {-1, 1, 3}
};

static int nregion = sizeof(region) / 3;

// chars are ~dist scaled by 2
char ambient_occlusion(TileMap* map, int xx, int yy) {
  int zz;
  char result = 0;
  for(zz = 0; zz < nregion; ++zz) {
    int ox = region[zz][0];
    int oy = region[zz][1];
    int incr = region[zz][2];

    TilePosition tpos = {xx + ox, yy + oy};
    if(!map->validindex(tpos)) {
      // the world edges are occluders
      result += incr;
      continue;
    }

    int8_t kind = map->tiles[map->index(tpos)];
    if(map->tile_specs[kind].bitmask & TILESPEC_VISIBLE) {
      result += incr;
    }
  }
  return result;
}

void charimage_ambient_occlusion(CharImage occlusion, TileMap* map) {
  int xx, yy;
  for(yy = 0; yy < map->height_IT; ++yy) {
    for(xx = 0; xx < map->width_IT; ++xx) {
      int ridx = charimage_index(occlusion, xx, yy);
      occlusion->data[ridx] = ambient_occlusion(map, xx, yy);
    }
  }
}
