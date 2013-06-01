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

#define min_time 1
#define max_time 100

static GameStep game_step_fn;
static float smoothed_delta_realtime_ms = 17.5;
static float movaverage_delta_time_ms = smoothed_delta_realtime_ms;
static long last_time_ms = 0;
#define movaverage_period 40.0f
#define smooth_factor 0.1f

void set_game_step(GameStep fn) {
  game_step_fn = fn;
}

Timer_ timer;

int loop_once() {
  long curr_time_ms;

  struct InputState_ state;
  inputstate_latest(&state);
  if(!game_step_fn || state.quit_requested) {
    lib_shutdown();
    return 0;
  }

  begin_frame();

  PROFILE_START(&timer, "main_update");
  game_step_fn(roundf(smoothed_delta_realtime_ms));
  PROFILE_END(&timer);

  end_frame();

  /* check the time */
  curr_time_ms = time_millis();

  /* smooth it out using a moving average */
  float realtime_elapsed_ms;
  if(last_time_ms > 0) {
    realtime_elapsed_ms = curr_time_ms - last_time_ms;
  } else {
    realtime_elapsed_ms = smoothed_delta_realtime_ms;
  }
  movaverage_delta_time_ms =
    (realtime_elapsed_ms + movaverage_delta_time_ms * (movaverage_period - 1))
    / movaverage_period;
  smoothed_delta_realtime_ms = smoothed_delta_realtime_ms +
    (movaverage_delta_time_ms - smoothed_delta_realtime_ms) * smooth_factor;
  last_time_ms = curr_time_ms;

  if(smoothed_delta_realtime_ms > max_time) {
    smoothed_delta_realtime_ms = max_time;
  }

  return 1;
}

int real_main(int argc, char ** argv) {
  lib_init(argc, argv);
  game_init(argc, argv);

  while(loop_once()) {}

  game_shutdown();
  return 0;
}
