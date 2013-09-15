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
#ifndef VECTOR_H
#define VECTOR_H

typedef struct Vector_ {
  float x;
  float y;
} *Vector;

void vector_add(Vector dst, const Vector_* a, const Vector_* b);
void vector_sub(Vector dst, const Vector_* a, const Vector_* b);
void vector_scale(Vector dst, const Vector_* a, float s);
float vector_dot(const Vector_* a, const Vector_* b);
float vector_mag(const Vector_* a);
void vector_zero(Vector a);
void vector_norm(Vector dst, const Vector_* src);
float vector_angle(const Vector_* v);
void vector_for_angle(Vector v, float angle);
int vector_direction_scaled(Vector dst, const Vector_* a, const Vector_* b, float s);
void vector_integrate(Vector dst, Vector r, Vector dr, float dt);
void vector_clamp(Vector dst, Vector src, float max);
float vector_scalarproject(Vector src, Vector onto);
float vector_project2(Vector dst, Vector src, Vector normonto);
float vector_dist2(const Vector_* a, const Vector_* b);
float vector_dist(const Vector_* a, const Vector_* b);

#endif
