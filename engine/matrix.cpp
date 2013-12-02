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
#include "matrix.h"
#include "giggle.h"

#include <math.h>

OBJECT_IMPL(Matrix44, Object);

Matrix44::Matrix44(void* _empty) {
  if(_empty) {
    *this = *(Matrix44*)_empty;
  }
}

Matrix44::Matrix44() {
}

Matrix44& Matrix44::operator=(const Matrix44& other) {
  memcpy(data, other.data, sizeof(data));
  return *this;
}

void Matrix44::identity() {
  data[0] = 1.0f;
  data[1] = 0.0f;
  data[2] = 0.0f;
  data[3] = 0.0f;

  data[4] = 0.0f;
  data[5] = 1.0f;
  data[6] = 0.0f;
  data[7] = 0.0f;

  data[8] = 0.0f;
  data[9] = 0.0f;
  data[10] = 1.0f;
  data[11] = 0.0f;

  data[12] = 0.0f;
  data[13] = 0.0f;
  data[14] = 0.0f;
  data[15] = 1.0f;
}
OBJECT_METHOD(Matrix44, identity, void, ());

void Matrix44::rotation(float angle) {
  float s = sinf(angle);
  float c = cosf(angle);

  data[0] = c;
  data[1] = s;
  data[2] = 0.0f;
  data[3] = 0.0f;

  data[4] = -s;
  data[5] = c;
  data[6] = 0.0f;
  data[7] = 0.0f;

  data[8] = 0.0f;
  data[9] = 0.0f;
  data[10] = 1.0f;
  data[11] = 0.0f;

  data[12] = 0.0f;
  data[13] = 0.0f;
  data[14] = 0.0f;
  data[15] = 1.0f;
}
OBJECT_METHOD(Matrix44, rotation, void, (float));

void Matrix44::orthographic_proj(float xmin, float xmax, float ymin, float ymax,
                                 float zmin, float zmax) {
  identity();
  data[0] = 2.0f / (xmax - xmin);
  data[5] = 2.0f / (ymax - ymin);
  data[10] = -2.0f / (zmax - zmin);
  data[12] = -((xmax + xmin)/(xmax - xmin));
  data[13] = -((ymax + ymin)/(ymax - ymin));
  data[14] = -((zmax + zmin)/(zmax - zmin));
  data[15] = 1.0f;
}
OBJECT_METHOD(Matrix44, orthographic_proj, void, (float, float, float, float, float, float))


Matrix44 Matrix44::operator*(const Matrix44& o) {
  Matrix44 result;
  for(unsigned ii = 0; ii < 4; ++ii) {
    for(unsigned jj = 0; jj < 4; ++jj) {
      result.elm(ii,jj) = 0;
      for(unsigned kk = 0; kk < 4; ++kk) {
        result.elm(ii,jj) += elm(ii,kk) * o.elm(kk,jj);
      }
    }
  }
  return result;
}


Matrix44* Matrix44::multiply(Matrix44* o) {
  Matrix44 temp = (*this) * (*o);
  Matrix44* result = new Matrix44(&temp);
  result->reference_count = 0; // disown
  return result;
}
OBJECT_METHOD(Matrix44, multiply, Matrix44*, (Matrix44*));

void Matrix44::vmult(Vector_* r, Vector_* v) {
  r->x = data[0] * v->x + data[4] * v->y;
  r->y = data[1] * v->x + data[5] * v->y;
}
