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
#include <stdio.h>
#include "giggle.h"
#include "giggle_sdl.h"
#include "testlib_internal.h"
#include "game.h"
#include "utils.h"
#include "config.h"

#include <SDL2/SDL.h>

// required by things that read files
FILE* nativeOpen(const char* fname) {
  return fopen(fname, "rb");
}

static Thread game_thread;

int main(int argc, char ** argv) {
  Giggle* giggle = lib_init(argc, argv);
  giggle->renderer = sdl_renderer(giggle);

  giggle->renderer->initialize();

  game_init(argc, argv);
  game_thread = thread_create(game_exec, NULL);

  // renderer claims main thread
  giggle->renderer->run();

  return 0;
}
