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
#include "vector.h"
#include <math.h>

void vector_add(Vector dst, Vector a, Vector b) {
  dst->x = a->x + b->x;
  dst->y = a->y + b->y;
}

void vector_sub(Vector dst, Vector a, Vector b) {
  dst->x = a->x - b->x;
  dst->y = a->y - b->y;
}

void vector_scale(Vector dst, Vector a, float s) {
  dst->x = a->x * s;
  dst->y = a->y * s;
}

float vector_dot(Vector a, Vector b) {
  float x = a->x * b->x;
  float y = a->y * b->y;
  return x + y;
}

float vector_mag(Vector a) {
  return sqrtf(vector_dot(a, a));
}

void vector_zero(Vector a) {
  a->x = 0.0f;
  a->y = 0.0f;
}

void vector_norm(Vector dst, Vector src) {
  vector_scale(dst, src, 1.0f / vector_mag(src));
}

float vector_angle(Vector v) {
  float angle = atan2(v->y, v->x);
  return angle;
}

void vector_for_angle(Vector v, float angle) {
  v->x = cosf(angle);
  v->y = sinf(angle);
}

int vector_direction_scaled(Vector dst, Vector a, Vector b, float s) {
  struct Vector_ ba;
  vector_sub(&ba, a, b);

  float mag = vector_mag(&ba);
  if(mag < 0.01) return 0;

  vector_scale(dst, &ba, s / mag);
  return 1;
}

void vector_integrate(Vector dst, Vector r, Vector dr, float dt) {
  dst->x = r->x + (dr->x * dt);
  dst->y = r->y + (dr->y * dt);
}

void vector_clamp(Vector dst, Vector src, float max) {
  float mag = vector_mag(src);
  if(mag > max) {
    vector_scale(dst, src, max / mag);
  } else {
    *dst = *src;
  }
}

float vector_scalarproject(Vector src, Vector onto) {
  struct Vector_ normonto;
  vector_norm(&normonto, onto);
  return vector_dot(src, &normonto);
}

// compute the vector and scalar projection of SRC onto NORMONTO
float vector_project2(Vector dst, Vector src, Vector normonto) {
  float scalarproj = vector_dot(src, normonto);
  vector_scale(dst, normonto, scalarproj);
  return scalarproj;
}

float vector_dist2(Vector a, Vector b) {
  float dx = a->x - b->x;
  float dy = a->y - b->y;
  return dx * dx + dy * dy;
}

float vector_dist(Vector a, Vector b) {
  return sqrtf(vector_dist2(a, b));
}
