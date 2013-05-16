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
#include "random.h"
#include "config.h"

#include <math.h>
#include <limits>

const uint32_t uint32_max = std::numeric_limits<uint32_t>::max();

int rand_in_range(Random random, int lower, int upper) {
  int range = upper - lower;
  if(range == 0) return upper;

  uint32_t rval = random_next_uint32(random);

  return lower + (rval % range);
}

float rand_in_rangef(Random random, float lower, float upper) {
  float range = upper - lower;
  if(range == 0.0f) return upper;

  double rval = (double)random_next_uint32(random);
  float uval = (float)(rval / uint32_max);
  return lower + uval * range;
}

void random_init(Random random, uint32_t seed) {
  sfmt_init_gen_rand((sfmt_t*)random, seed);
  random->phase = 0;
}

// Abramowitz and Stegun: http://c-faq.com/lib/gaussian.html
float random_next_gaussian(Random random) {
  float z;

  if(random->phase == 0) {
    random->u = (random_next_uint32(random) + 1.0f) / (uint32_max + 2.0f);
    random->v = random_next_uint32(random) / (uint32_max + 1.0f);
    z = sqrt(-2.0f * logf(random->u)) * sinf(2.0f * M_PI * random->v);
  } else {
    z = sqrt(-2.0f * logf(random->u)) * cosf(2.0f * M_PI * random->v);
  }

  random->phase = 1 - random->phase;

  return z;
}

void random_shuffle_bytes(Random random, unsigned char* bytes, int nbytes) {
  int ii;
  for(ii = 0; ii < nbytes - 1; ++ii) {
    int swap_tgt = rand_in_range(random, ii + 1, nbytes);
    unsigned char temp = bytes[ii];
    bytes[ii] = bytes[swap_tgt];
    bytes[swap_tgt] = temp;
  }
}
