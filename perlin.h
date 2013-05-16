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
#ifndef PERLIN_H
#define PERLIN_H

#include "random.h"
#include "vector.h"

typedef struct Perlin_ {
  struct Vector_ offset, scale;
  float z;
  unsigned char state[512];
} *Perlin;

void perlin_init(Perlin perlin, Random rng, Vector offset, Vector scale);

float perlin_sample(Perlin perlin, Vector point);

#endif
