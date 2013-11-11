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
#ifndef SPRITEATLAS_H
#define SPRITEATLAS_H

#include "giggle.h"
#include "listlib.h"

#include <stdint.h>

#define MAX_ENTRY_NAME 32

struct SpriteAtlas_;

typedef struct SpriteAtlasEntry_ {
  struct SpriteAtlas_* atlas;
  unsigned short w, h;
  float u0, v0, u1, v1;
  char name[MAX_ENTRY_NAME];
} *SpriteAtlasEntry;

typedef struct SpriteAtlas_ {
  ImageResource image;
  int nentries;
  struct SpriteAtlasEntry_ entries[0];
} *SpriteAtlas;

SpriteAtlas spriteatlas_load(const char* name, const char* imgtype);
void spriteatlas_free(SpriteAtlas atlas);
int spriteatlas_index(SpriteAtlasEntry entry);
SpriteAtlasEntry spriteatlas_find(SpriteAtlas atlas, const char* name);

void sprite_fillfromentry(BaseSprite sprite, SpriteAtlasEntry entry);

#endif
