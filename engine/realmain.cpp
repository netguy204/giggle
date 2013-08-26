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
#include "testlib.h"
#include "game.h"
#include "utils.h"
#include "config.h"

#define min_time 1
#define max_time 100

static GameStep game_step_fn;
static float last_frame_ms = (1.0 / 60.0) * 1e3;

void set_game_step(GameStep fn) {
  game_step_fn = fn;
}

Timer_ timer;

int loop_once() {
  struct InputState_ state;
  inputstate_latest(&state);
  if(!game_step_fn || state.quit_requested) {
    lib_shutdown();
    return 0;
  }

  float start_ms = time_millis();

  begin_frame();

  PROFILE_START(&timer, "main_update");
  game_step_fn(roundf(last_frame_ms));
  PROFILE_END(&timer);

  end_frame();

  last_frame_ms = time_millis() - start_ms;
  last_frame_ms = MAX(min_time, MIN(max_time, last_frame_ms));

  return 1;
}

int real_main(int argc, char ** argv) {
  lib_init(argc, argv);
  game_init(argc, argv);

  while(loop_once()) {}

  game_shutdown();
  return 0;
}
