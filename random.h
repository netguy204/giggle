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
#ifndef RANDOM_H
#define RANDOM_H

#define SFMT_MEXP 607
#include "SFMT.h"

typedef struct Random_ {
  sfmt_t sfmt;
  uint32_t phase;
  float u, v;
} *Random;

#define random_next_uint32(random) sfmt_genrand_uint32((sfmt_t*)random)

void random_init(Random random, uint32_t seed);
int rand_in_range(Random random, int lower, int upper);
float rand_in_rangef(Random random, float lower, float upper);
float random_next_gaussian(Random random);
void random_shuffle_bytes(Random random, unsigned char* bytes, int nbytes);

#endif
