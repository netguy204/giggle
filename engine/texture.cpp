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
#include "texture.h"
#include "testlib_gl.h"

Texture::Texture()
  : texid(0) {
}

Texture::~Texture() {
  if(texid != 0) {
    glDeleteTextures(1, &texid);
  }
}

void Texture::bind() {
  if(texid != current_texture) {
    gl_check(glBindTexture(GL_TEXTURE_2D, texid));
    current_texture = texid;
  }
}

void Texture::unbind() {
  glBindTexture(GL_TEXTURE_2D, 0);
  current_texture = 0;
}

GLuint Texture::current_texture = 0;
