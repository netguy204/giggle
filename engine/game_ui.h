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
#include "gameobject.h"

class Font : public Object {
 public:
  OBJECT_PROTO(Font);
  Font(void* world);

  void load(SpriteAtlas atlas, const char* prefix, const char* character_map);
  const char* wrap_string(const char* string, int width);

  int char_xadvance(char ch);
  int char_yadvance(char ch);
  int char_xlead(char ch);
  int char_ylead(char ch);

  void set_char_xadvance(char ch, int w);
  void set_char_yadvance(char ch, int w);
  void set_char_xlead(char ch, int w);
  void set_char_ylead(char ch, int w);

  int line_height();
  int string_width(const char* string);
  int string_height(const char* string);

  SpriteAtlasEntry entry(char ch);

  int word_separation;
  int line_separation;
  float scale;

  SpriteAtlasEntry table[256];
  int xadvance[256];
  int yadvance[256];

  int xleads[256];
  int yleads[256];

  World* world;
};

class CDrawText : public Component {
 public:
  OBJECT_PROTO(CDrawText);
  CDrawText(void* go);
  virtual ~CDrawText();

  virtual void render(Camera* camera);
  void set_font(Font* font);
  Font* get_font();

  Vector_ offset;
  LString* message;
  Font* font;
  Color color;
  int layer;
};

BaseSprite spritelist_from_8patch(BaseSprite list, SpriteAtlas atlas,
                                  Rect screen_rect);

BaseSprite spritelist_from_string(BaseSprite list, Font* font,
                                  const char* string, int bl_x, int bl_y,
                                  Color* c);


void gameui_submit();


#endif
