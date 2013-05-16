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
#ifndef RECT_H
#define RECT_H

#include "vector.h"

typedef struct Rect_ {
  float minx, miny, maxx, maxy;
} *Rect;

float rect_width(Rect rect);
float rect_height(Rect rect);
int rect_intersect(Rect a, Rect b);
void rect_centered(Rect rect, Vector pos, float w, float h);
void rect_offset(Rect output, Rect input, Vector offset);
void rect_center(Vector center, Rect rect);
void rect_scaled(Rect output, Rect input, float w, float h);

#endif
