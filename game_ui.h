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
#ifndef GAME_UI_H
#define GAME_UI_H

#include "testlib.h"
#include "spriteatlas.h"
#include "color.h"

typedef enum {
  FONT_SMALL,
  FONT_MEDIUM,
  FONT_FIXED,
  FONT_MAX
} FontSize;

BaseSprite spritelist_from_8patch(BaseSprite list, SpriteAtlas atlas,
                                  Rect screen_rect);

BaseSprite spritelist_from_string(BaseSprite list, SpriteAtlas atlas, FontSize size,
                                  const char* string, int bl_x, int bl_y,
                                  Color* c);

BaseSprite spritelist_from_consoletext(BaseSprite list, SpriteAtlas atlas, const char* string,
                                       int bl_x, int bl_y, int width, Color *c);

void gameui_submit();

#endif
