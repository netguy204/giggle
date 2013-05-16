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
#include "controls.h"
#include "testlib.h"

int repeatinglatch_state(RepeatingLatch latch, Clock clock, int input_state) {
  if(latch->last_state != input_state) {
    latch->last_state = input_state;
    latch->last_time = clock_time(clock);
    return input_state;
  } else {
    // has the clock expired?
    long dt_ticks = clock_time(clock) - latch->last_time;
    float dt = clock_cycles_to_seconds(dt_ticks);
    if(dt >= latch->period) {
      latch->last_time = clock_time(clock);
      return input_state;
    } else {
      // return the default value
      return latch->latch_value;
    }
  }
}
