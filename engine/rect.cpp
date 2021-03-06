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
#include "rect.h"

float rect_width(Rect rect) {
  return rect->maxx - rect->minx;
}

float rect_height(Rect rect) {
  return rect->maxy - rect->miny;
}

int rect_intersect(Rect a, Rect b) {
  if (a->maxx < b->minx) return 0;
  if (a->minx > b->maxx) return 0;
  if (a->maxy < b->miny) return 0;
  if (a->miny > b->maxy) return 0;
  return 1;
}

void rect_centered(Rect rect, Vector pos, float w, float h) {
  float hw = w / 2.0f;
  float hh = h / 2.0f;
  rect->minx = pos->x - hw;
  rect->maxx = pos->x + hw;
  rect->miny = pos->y - hh;
  rect->maxy = pos->y + hh;
}

void rect_offset(Rect output, Rect input, Vector offset) {
  output->minx = input->minx + offset->x;
  output->maxx = input->maxx + offset->x;
  output->miny = input->miny + offset->y;
  output->maxy = input->maxy + offset->y;
}

void rect_center(Vector center, Rect rect) {
  center->x = (rect->minx + rect->maxx) / 2.0f;
  center->y = (rect->miny + rect->maxy) / 2.0f;
}

void rect_scaled(Rect output, Rect input, float sw, float sh) {
  struct Vector_ center;
  rect_center(&center, input);
  rect_centered(output, &center, rect_width(input) * sw,
                rect_height(input) * sh);
}
