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
#ifndef MATRIX_H
#define MATRIX_H

#include "ooc.h"
#include "vector.h"

class Matrix44 : public Object {
 public:
  OBJECT_PROTO(Matrix44);
  Matrix44(void* _empty);
  Matrix44();

  inline float& elm(unsigned r, unsigned c) {
    return data[c*4+r];
  }

  inline float elm(unsigned r, unsigned c) const {
    return data[c*4+r];
  }

  Matrix44& operator=(const Matrix44& other);
  Matrix44 operator*(const Matrix44& other);

  void identity();
  void rotation(float a);

  Matrix44* multiply(Matrix44* other);
  void vmult(Vector_* r, Vector_* v);

  void orthographic_proj(float xmin, float xmax, float ymin, float ymax,
                         float zmin, float zmax);

  float data[16];
};

#endif
