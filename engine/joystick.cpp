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
#include <fcntl.h>
#include <errno.h>
#include <memory.h>
#include <unistd.h>

#include "joystick.h"

const char* type2str(__u8 type) {
  type &= ~JS_EVENT_INIT;
  switch(type) {
  case JS_EVENT_BUTTON:
    return "JS_EVENT_BUTTON";
    break;
  case JS_EVENT_AXIS:
    return "JS_EVENT_AXIS";
    break;
  case JS_EVENT_INIT:
    return "JS_EVENT_INIT";
    break;
  default:
    return "UNKNOWN";
  }
}

void js_event_print(struct js_event* e) {
  printf("%s: time=%d value=%d number=%d\n", type2str(e->type),
	 e->time, e->value, e->number);
}

int joystick_maybe_read(int fileno, struct js_event* e) {
  if(read(fileno, e, sizeof(struct js_event)) <= 0) {
    if(errno != EAGAIN) {
      fprintf(stderr, "error reading from joystick\n");
      exit(-1);
    }
    return 0;
  } else {
    e->type &= ~JS_EVENT_INIT;
    return 1;
  }
}

js_state joystick_open(const char* device) {
  int fd = open ("/dev/input/js0", O_RDONLY|O_NONBLOCK);

  int capacity = 32;
  js_state state = (js_state)malloc(sizeof(struct js_state_));
  state->num_values = 0;
  state->capacity = capacity;
  state->fileno = fd;
  state->values = (js_event*)malloc(capacity * sizeof(struct js_event));
  memset(state->values, 0, capacity * sizeof(struct js_event));

  joystick_update_state(state);
  return state;
}

void joystick_close(js_state state) {
  close(state->fileno);
  free(state->values);
  free(state);
}

void joystick_update_state(js_state state) {
  struct js_event e;

  while(joystick_maybe_read(state->fileno, &e)) {
    int slot = e.number * 2 + e.type - 1;
    if(slot >= state->num_values) {
      if(slot >= state->capacity) {
	int new_cap = slot + 1;
	state->values = (js_event*)realloc(state->values, new_cap * sizeof(struct js_event));
	state->capacity = new_cap;
      }

      state->num_values = slot + 1;
    }

    state->values[slot] = e;
  }
}

void joystick_print_state(js_state state) {
  int ii;
  for(ii = 0; ii < state->num_values; ++ii) {
    js_event_print(state->values + ii);
  }
}

/*
int main(int argc, char** argv) {
  int ii;
  js_state state = joystick_open("/dev/input/js0");  

  joystick_print_state(state);

  joystick_close(state);

  return 1;
}
*/
