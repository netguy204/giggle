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
#include "spriter.h"
#include "spriteatlas.h"

int main(int argc, char *argv[])
{
  testlib_init();
  SpriteAtlas atlas = spriteatlas_load("resources/images_default", "png");
  Entity* ent = spriter_load("./out", atlas);
  Animation* anim = spriter_find(ent, "First Animation");
  Vector_ pos = {0.0f, 0.0f};
  BaseSprite list = spriter_append(NULL, anim, &pos, 50);
  return 0;
}
