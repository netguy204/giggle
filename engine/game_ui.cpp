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
#include "game_ui.h"
#include "heapvector.h"

#include "config.h"

#include <stdio.h>
#include <ctype.h>

#include <vector>

OBJECT_IMPL(CDrawText, Component);
OBJECT_PROPERTY(CDrawText, message);
OBJECT_ACCESSOR(CDrawText, font, get_font, set_font);
OBJECT_PROPERTY(CDrawText, layer);
OBJECT_PROPERTY(CDrawText, color);
OBJECT_PROPERTY(CDrawText, offset);

CDrawText::CDrawText(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), message(NULL), font(NULL), layer(LAYER_MENU) {
  vector_zero(&offset);
  color.r = 1.0;
  color.g = 1.0;
  color.b = 1.0;
  color.a = 1.0;
}

CDrawText::~CDrawText() {
  if(message) free_lstring(message);
  if(font) font->release();
}

void CDrawText::set_font(Font* _font) {
  if(font) font->release();
  font = _font;
  font->retain();
}

Font* CDrawText::get_font() {
  return font;
}

void CDrawText::render(Camera* camera) {
  if(!message) return;
  if(!font) fail_exit("CDrawText was not given a font");

  BaseSprite* spritep = &camera->particles[layer];
  Vector_ pos;
  go->pos(&pos);

  vector_add(&pos, &pos, &offset);

  *spritep = spritelist_from_string(*spritep, font, message->str,
                                    pos.x, pos.y, &color);
}

typedef enum {
  EP_TL,
  EP_T,
  EP_TR,
  EP_L,
  EP_C,
  EP_R,
  EP_BL,
  EP_B,
  EP_BR,
  EP_MAX
} EightPatchElement;

const char* eight_patch_names[EP_MAX] = {
  "8patch_0",
  "8patch_1",
  "8patch_2",
  "8patch_3",
  "8patch_4",
  "8patch_5",
  "8patch_6",
  "8patch_7",
  "8patch_8"
};

OBJECT_IMPL(Font, Object);
OBJECT_PROPERTY(Font, word_separation);
OBJECT_PROPERTY(Font, line_separation);
OBJECT_PROPERTY(Font, scale);

Font::Font(void* _game)
  : scale(1.0), game((Game*)_game) {
}

void Font::load(SpriteAtlas atlas, const char* prefix, const char* character_map) {
  memset(table, 0, sizeof(table));
  memset(xadvance, 0, sizeof(xadvance));
  memset(yadvance, 0, sizeof(yadvance));
  memset(xleads, 0, sizeof(xleads));
  memset(yleads, 0, sizeof(yleads));
  memset(kerning, 0, sizeof(kerning));

  char tempname[128];
  int idx = 1;
  for(const char* ch = character_map; *ch != '\0'; ++ch) {
    snprintf(tempname, sizeof(tempname), "%s%d", prefix, idx++);
    table[(unsigned)*ch] = game->atlas_entry(atlas, tempname);
    xadvance[(unsigned)*ch] = table[(unsigned)*ch]->w;
  }

  word_separation = MAX(2, table[(unsigned)'m']->w/2);
  line_separation = table[(unsigned)'Q']->h;
}
OBJECT_METHOD(Font, load, void, (SpriteAtlas, const char*, const char*));

int Font::char_xadvance(char ch) {
  SpriteAtlasEntry entry = table[(unsigned)ch];
  if(entry == NULL) {
    return word_separation * scale;
  } else {
    return xadvance[(unsigned)ch] * scale;
  }
}
OBJECT_METHOD(Font, char_xadvance, int, (char));

int Font::char_yadvance(char ch) {
  SpriteAtlasEntry entry = table[(unsigned)ch];
  if(entry == NULL) {
    return 0;
  } else {
    return (yadvance[(unsigned)ch]) * scale;
  }
}
OBJECT_METHOD(Font, char_yadvance, int, (char));

int Font::char_xlead(char ch) {
  return xleads[(unsigned)ch] * scale;
}
OBJECT_METHOD(Font, char_xlead, int, (char));

int Font::char_ylead(char ch) {
  return yleads[(unsigned)ch] * scale;
}
OBJECT_METHOD(Font, char_ylead, int, (char));

void Font::set_char_xadvance(char ch, int w) {
  xadvance[(unsigned)ch] = w;
}
OBJECT_METHOD(Font, set_char_xadvance, void, (char, int));

void Font::set_char_yadvance(char ch, int w) {
  yadvance[(unsigned)ch] = w;
}
OBJECT_METHOD(Font, set_char_yadvance, void, (char, int));

void Font::set_char_xlead(char ch, int l) {
  xleads[(unsigned)ch] = l;
}
OBJECT_METHOD(Font, set_char_xlead, void, (char, int));

void Font::set_char_ylead(char ch, int l) {
  yleads[(unsigned)ch] = l;
}
OBJECT_METHOD(Font, set_char_ylead, void, (char, int));

int Font::get_kerning(char prev, char next) {
  return kerning[(unsigned)prev][(unsigned)next];
}
OBJECT_METHOD(Font, get_kerning, int, (char, char));

void Font::set_kerning(char prev, char next, int value) {
  kerning[(unsigned)prev][(unsigned)next] = value;
}
OBJECT_METHOD(Font, set_kerning, void, (char, char, int));

int Font::line_height() {
  return line_separation * scale;
}
OBJECT_METHOD(Font, line_height, int, ());

int Font::string_width(const char* string) {
  int max_width = 0;
  int width = 0;
  for(; *string; ++string) {
    if(*string == '\n') {
      max_width = MAX(max_width, width);
      width = 0;
    } else {
      width += char_xadvance(*string);
    }
  }

  return MAX(max_width, width);
}
OBJECT_METHOD(Font, string_width, int, (const char*));

int Font::string_height(const char* string) {
  int height = line_height();
  for(; *string; ++string) {
    if(*string == '\n') {
      height += line_height();
    }
  }
  return height;
}
OBJECT_METHOD(Font, string_height, int, (const char*));

static int is_whitespace(char ch) {
  return ch == ' ' || ch == '\n' || ch == '\t';
}

static std::vector<char> ws_output;

const char* Font::wrap_string(const char* input, int max_width) {
  int width = 0;
  const char* ip;

  ws_output.clear();

  while(*input) {
    // count up next word
    int word_width = 0;
    for(ip=input; *ip && !is_whitespace(*ip); ++ip) {
      word_width += char_xadvance(*ip);
    }

    // insert newline if needed
    if(width + word_width > max_width) {
      char newline = '\n';
      ws_output.push_back(newline);
      width = 0;
    }

    // copy the word
    for(;input != ip; ++input) {
      ws_output.push_back(*input);
    }
    width += word_width;

    // copy any spaces
    while(is_whitespace(*input)) {
      ws_output.push_back(*input);
      if(*input == '\n') {
        width = 0;
      } else {
        width += char_xadvance(*ip);
      }

      ++input;
    }
  }

  char eos = '\0';
  ws_output.push_back(eos);
  return &ws_output[0];
}
OBJECT_METHOD(Font, wrap_string, const char*, (const char*, int));

SpriteAtlasEntry patch_cache[EP_MAX];

static Sprite ui_make_sprite(SpriteAtlasEntry entry, int x, int y,
                             Color* c) {
  Sprite sprite = GIGGLE->make_sprite();
  sprite_fillfromentry(sprite, entry);
  sprite->displayX = floor(x);
  sprite->displayY = floor(y);
  sprite->originX = 0;
  sprite->originY = 0;
  sprite->angle = 0;
  sprite->color[0] = c->r;
  sprite->color[1] = c->g;
  sprite->color[2] = c->b;
  sprite->color[3] = c->a;
  return sprite;
}

BaseSprite spritelist_from_8patch(BaseSprite list, SpriteAtlas atlas, Rect rect) {
  int major_dim = patch_cache[EP_C]->w;
  int xsteps = (rect_width(rect) / major_dim) + 1;
  int ysteps = (rect_height(rect) / major_dim) + 1;
  int xpos = rect->minx;
  int ypos = rect->miny;

  int row, col;
  for(row = 0; row < ysteps; ++row) {
    int ymax = 0;
    for(col = 0; col < xsteps; ++col) {
      EightPatchElement element;

      if(row == 0) {
        if(col == 0) {
          element = EP_BL;
        } else if(col == (xsteps - 1)) {
          element = EP_BR;
        } else {
          element = EP_B;
        }
      } else if(row == (ysteps - 1)) {
        if(col == 0) {
          element = EP_TL;
        } else if(col == (xsteps - 1)) {
          element = EP_TR;
        } else {
          element = EP_T;
        }
      } else if(col == 0) {
        element = EP_L;
      } else if(col == (xsteps - 1)) {
        element = EP_R;
      } else {
        element = EP_C;
      }

      SpriteAtlasEntry entry = patch_cache[element];
      Color c = {1.0f, 1.0f, 1.0f, 1.0f};
      Sprite sprite = ui_make_sprite(entry, xpos, ypos, &c);
      sprite_append(list, sprite);
      xpos += entry->w;
      ymax = MAX(ymax, entry->h);
    }

    ypos += ymax;
    xpos = rect->minx;
  }

  return list;
}

int is_number(char ch) {
  return (ch >= '0' && ch <= '9') || ch == '.';
}

BaseSprite spritelist_from_string(BaseSprite list, Font* font, const char* string,
                                  int bl_x, int bl_y, Color* ic) {
  Color c = *ic;
  char shade_buffer[12];

  int xstart = bl_x;
  const char* prev = NULL;

  for(; *string; prev = string++) {
    if(*string == '\n') {
      bl_y -= font->line_height();
      bl_x = xstart;
      continue;
    }

    // look for a shading code
    if(*string == '#' && *(string + 1) == '~') {
      // skip prefix
      string += 2;
      int idx = 0;
      while(is_number(*string)) {
        shade_buffer[idx++] = *string;
        ++string;
      }
      shade_buffer[idx] = '\0';
      float shade = atof(shade_buffer);
      c.r = ic->r * shade;
      c.g = ic->g * shade;
      c.b = ic->b * shade;
      --string; // give back last char
      continue;
    }

    SpriteAtlasEntry entry = font->table[(unsigned)*string];
    if(entry) {
      // apply kerning first
      if(prev) {
        bl_x += font->get_kerning(*prev, *string);
      }

      Sprite sprite = ui_make_sprite(entry, bl_x + font->char_xlead(*string),
                                     bl_y + font->char_ylead(*string), &c);
      sprite->w *= font->scale;
      sprite->h *= font->scale;
      sprite_append(list, sprite);
    }

    bl_x += font->char_xadvance(*string);
    bl_y += font->char_yadvance(*string);
  }

  return list;
}

void gameui_submit() {

}
