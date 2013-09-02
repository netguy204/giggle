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
#ifndef FILENUMBERS_H
#define FILENUMBERS_H

#include <stdio.h>

void read_ushort(FILE* fh, unsigned short* value);

void read_ushorts(FILE* fh, unsigned short* values, unsigned n);

void read_short(FILE* fh, short* value);

// a normalized fixed number works best for values between 0 and 1
void read_norm_fixed(FILE* fh, float* value);

// a non-normalized fixed number works for floats that are +/- about 250
void read_fixed(FILE* fh, float* value);

// read a 32 byte string
void read_fstring(FILE* f, char* buffer);

#endif
