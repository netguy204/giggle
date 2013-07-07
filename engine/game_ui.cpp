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

OBJECT_IMPL(CDrawText, Component);
OBJECT_PROPERTY(CDrawText, message);
OBJECT_ACCESSOR(CDrawText, font, get_font, set_font);
OBJECT_PROPERTY(CDrawText, layer);
OBJECT_PROPERTY(CDrawText, color);
OBJECT_PROPERTY(CDrawText, offset);

CDrawText::CDrawText(void* _go)
  : Component((GO*)_go, PRIORITY_SHOW), layer(LAYER_MENU), message(NULL), font(NULL) {
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

OBJECT_IMPL(CDrawConsoleText, CDrawText);
OBJECT_PROPERTY(CDrawConsoleText, w);

CDrawConsoleText::CDrawConsoleText(void *_go)
  : CDrawText(_go) {
  w = 100;
}

void CDrawConsoleText::render(Camera* camera) {
  if(!message) return;
  if(!font) fail_exit("CDrawConsoleText was not given a font atlas");

  BaseSprite* spritep = &camera->particles[layer];
  Vector_ pos;
  go->pos(&pos);

  vector_add(&pos, &pos, &offset);

  *spritep = spritelist_from_consoletext(
        *spritep, font, message->str, pos.x, pos.y,
        w, &color);
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
OBJECT_PROPERTY(Font, character_separation);
OBJECT_PROPERTY(Font, word_separation);
OBJECT_PROPERTY(Font, line_separation);
OBJECT_PROPERTY(Font, scale);

Font::Font(void* _world)
  : scale(1.0) {
  world = (World*)_world;
  memset(table, 0, sizeof(table));
}

void Font::load(SpriteAtlas atlas, const char* prefix, const char* character_map) {
  memset(table, 0, sizeof(table));

  char tempname[128];
  int idx = 1;
  for(const char* ch = character_map; *ch != '\0'; ++ch) {
    snprintf(tempname, sizeof(tempname), "%s%d", prefix, idx++);
    table[*ch] = world->atlas_entry(atlas, tempname);
  }

  word_separation = MAX(2, table['m']->w/2);
  character_separation = MAX(1, word_separation/2);
  line_separation = table['Q']->h + character_separation;
}
OBJECT_METHOD(Font, load, void, (SpriteAtlas, const char*, const char*));

SpriteAtlasEntry patch_cache[EP_MAX];

static Sprite ui_make_sprite(SpriteAtlasEntry entry, int x, int y,
                             Color* c) {
  Sprite sprite = frame_make_sprite();
  sprite_fillfromentry(sprite, entry);
  sprite->displayX = x;
  sprite->displayY = y;
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

static int ui_ord(char ch) {
  if(ch >= 'a' && ch <= 'z') return ch - 'a';
  if(ch >= 'A' && ch <= 'z') return 26 + (ch - 'A');
  if(ch >= '0' && ch <= '9') return 52 + (ch - '0');
  return -1;
}

int ui_char_width(Font* font, char ch) {
  SpriteAtlasEntry entry = font->table[ch];
  if(entry == NULL) {
    return font->word_separation * font->scale;
  } else {
    return (entry->w + font->character_separation) * font->scale;
  }
}

int ui_line_height(Font* font) {
  return font->line_separation * font->scale;
}

int ui_string_width(Font* font, const char* string) {
  int width = 0;
  for(; *string; ++string) {
    width += ui_char_width(font, *string);
  }

  return width;
}

int is_whitespace(char ch) {
  return ch == ' ' || ch == '\n' || ch == '\t';
}

void ui_wrap_string(HeapVector output, const char* input, int max_chars) {
  const char* ip;
  int xpos;

  while(*input) {
    // count up next word
    for(ip=input; *ip && !is_whitespace(*ip); ++ip);

    // insert newline if needed
    if(xpos + (ip - input) > max_chars) {
      char newline = '\n';
      HV_PUSH_VALUE(output, char, newline);
      xpos = 0;
    }

    // copy the word
    for(;input != ip; ++input) {
      HV_PUSH_VALUE(output, char, *input);
      ++xpos;
    }

    // copy any spaces
    while(is_whitespace(*input)) {
      HV_PUSH_VALUE(output, char, *input);
      if(*input == '\n') {
        xpos = 0;
      } else {
        ++xpos;
      }

      ++input;
    }
  }

  char eos = '\0';
  HV_PUSH_VALUE(output, char, eos);
}

int is_number(char ch) {
  return (ch >= '0' && ch <= '9') || ch == '.';
}

BaseSprite spritelist_from_string(BaseSprite list, Font* font, const char* string,
                                  int bl_x, int bl_y, Color* ic) {
  Color c = *ic;
  char shade_buffer[12];

  int xstart = bl_x;

  for(; *string; ++string) {
    if(*string == '\n') {
      bl_y -= ui_line_height(font);
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

    SpriteAtlasEntry entry = font->table[*string];
    if(entry) {
      Sprite sprite = ui_make_sprite(entry, bl_x, bl_y, &c);
      sprite->w *= font->scale;
      sprite->h *= font->scale;
      sprite_append(list, sprite);
    }

    bl_x += ui_char_width(font, *string);
  }

  return list;
}

HeapVector consoletext_hv = NULL;

BaseSprite spritelist_from_consoletext(
      BaseSprite list, Font* font, const char* string,
      int bl_x, int bl_y, int width, Color* c) {

  int char_width = ui_char_width(font, 'm');
  int num_chars = width / char_width;

  if(consoletext_hv == NULL) {
    consoletext_hv = heapvector_make();
  }

  ui_wrap_string(consoletext_hv, string, num_chars);
  list = spritelist_from_string(list, font, consoletext_hv->data, bl_x, bl_y, c);
  heapvector_clear(consoletext_hv);
  return list;
}

void gameui_submit() {

}
