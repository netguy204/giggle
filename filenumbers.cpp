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
#include "filenumbers.h"

#ifdef __ANDROID__
#include <sys/endian.h>
#else
#ifdef WINDOWS
short ntohs(short val) {
  return ((val & 0xFF) << 8)
    | ((val >> 8) & 0xFF);
}

int ntohl(int val) {
  return ((val & 0xFF) << 24)
    | ((val & 0xFF00) << 8)
    | ((val >> 8) & 0xFF00)
    | ((val >> 24) & 0xFF);
}

#else
#include <arpa/inet.h>
#endif
#endif

#define NORM_COORD_SCALE (1 << 15)
#define COORD_SCALE (1 << 15)

void read_ushort(FILE* fh, unsigned short* value) {
  fread(value, sizeof(unsigned short), 1, fh);
  *value = ntohs(*value);
}

void read_short(FILE* fh, short* value) {
  read_ushort(fh, (unsigned short*)value);
}

void read_int(FILE* fh, int* value) {
  fread(value, sizeof(int), 1, fh);
  *value = ntohl(*value);
}

void read_norm_fixed(FILE* fh, float* value) {
  short fixed_value;
  read_short(fh, &fixed_value);
  *value = ((double)fixed_value) / NORM_COORD_SCALE;
}

void read_fixed(FILE* fh, float* value) {
  int fixed_value;
  read_int(fh, &fixed_value);
  *value = ((double)fixed_value) / COORD_SCALE;
}
