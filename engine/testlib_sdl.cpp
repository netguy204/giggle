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
/* SDL/OpenGL implementation of testlib suitable for desktops */

#include <SDL/SDL.h>

#include "testlib.h"
#include "testlib_internal.h"
#include "gl_headers.h"
#include "utils.h"
#include "input.h"
#include "testlib_gl.h"

#include <math.h>

// remap the SDL keycodes to our internal keycodes
static int sdl_remap_table[SDLK_LAST];
static InputState_ pstate;

void native_init() {
  /* appears to be the ouya default resolution
  screen_width = 1920;
  screen_height = 1080;
  */
  screen_width = 1280;
  screen_height = 720;

  /* default to unmapped */
  for(int ii = 0; ii < sizeof(sdl_remap_table) / sizeof(sdl_remap_table[0]); ++ii) {
    sdl_remap_table[ii] = -1;
  }

  /* remap the ASCII keys, everything is lowercase */
  for(int ii = 0; ii < (SDLK_z - SDLK_a + 1); ++ii) {
    sdl_remap_table[SDLK_a + ii] = 'a' + ii;
  }

  /* remap numbers */
  for(int ii = 0; ii < 10; ++ii) {
    sdl_remap_table[SDLK_0 + ii] = '0' + ii;
  }

  /* remap a few others manually */
  sdl_remap_table[SDLK_RETURN] = K_ENTER;
  sdl_remap_table[SDLK_TAB] = K_TAB;
  sdl_remap_table[SDLK_BACKSPACE] = K_BACKSPACE;
  sdl_remap_table[SDLK_SPACE] = K_SPACE;
  sdl_remap_table[SDLK_LMETA] = K_ALT;
  sdl_remap_table[SDLK_RMETA] = K_ALT;
  sdl_remap_table[SDLK_LALT] = K_ALT;
  sdl_remap_table[SDLK_RALT] = K_ALT;
  sdl_remap_table[SDLK_LSUPER] = K_ALT;
  sdl_remap_table[SDLK_RSUPER] = K_ALT;
}

KeyNumber mouse2kn(int mouse) {
  switch(mouse) {
  case SDL_BUTTON_LEFT:
    return K_MOUSE1;
  case SDL_BUTTON_MIDDLE:
    return K_MOUSE3;
  case SDL_BUTTON_RIGHT:
    return K_MOUSE2;
  default:
    return K_MOUSE4;
  }
}

void update_input_state() {
  SDL_Event event;

  /* pump the events */
  while(SDL_PollEvent(&event)) {
    int keydown = 0;

    switch(event.type) {
    case SDL_QUIT:
      pstate.quit_requested = 1;
      break;
    case SDL_KEYDOWN:
      keydown = 1;
      break;
    }

    if(event.type == SDL_KEYDOWN ||
       event.type == SDL_KEYUP) {
      switch(event.key.keysym.sym) {
      case SDLK_LEFT:
        handle_key(K_LEFTRIGHT, keydown * -1.0f);
        break;
      case SDLK_RIGHT:
        handle_key(K_LEFTRIGHT, keydown * 1.0f);
        break;
      case SDLK_DOWN:
        handle_key(K_UPDOWN, keydown * -1.0f);
        break;
      case SDLK_UP:
        handle_key(K_UPDOWN, keydown * 1.0f);
        break;
      default:
        int remapped = sdl_remap_table[event.key.keysym.sym];
        if(remapped >= 0) {
          handle_key((KeyNumber)remapped, keydown * 1.0f);
        }
        break;
      }
    } else if(event.type == SDL_MOUSEBUTTONDOWN) {
      handle_key(mouse2kn(event.button.button), 1.0f);
    } else if(event.type == SDL_MOUSEBUTTONUP) {
      handle_key(mouse2kn(event.button.button), 0.0f);
    } else if(event.type == SDL_MOUSEMOTION) {
      float xs = screen_width / (float)real_screen_width;
      float ys = screen_height / (float)real_screen_height;

      SpatialInput input;
      input.absolute.x = event.motion.x * xs;
      input.absolute.y = (real_screen_height - event.motion.y) * ys;
      input.relative.x = event.motion.xrel * xs;
      input.relative.y = -event.motion.yrel * ys;
      handle_spatialinput(SI_MOUSE, &input);
    }
  }
}

void inputstate_latest(InputState state) {
#ifndef WINDOWS
  update_input_state();
#endif

  *state = pstate;
}

long time_millis() {
  return SDL_GetTicks();
}

void sleep_millis(long millis) {
  SDL_Delay(millis);
}

void renderer_init(void* empty) {
  LOGI("renderer_init");
  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }

  /* probably supported in sdl > 1.2
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  */

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_Surface* screen = SDL_SetVideoMode(screen_width, screen_height,
                                         16, SDL_OPENGL);
  if(screen == NULL) {
    fprintf(stderr, "Unable to set %dx%d video: %s\n",
            screen_width, screen_height, SDL_GetError());
    exit(1);
  }

  GLenum err = glewInit();
  if(err != GLEW_OK) {
    fprintf(stderr, "Failed to initialize GLEW\n");
    exit(1);
  } else {
    LOGI("glew initialized");
  }

  SDL_WM_SetCaption("GGL", NULL);
  //SDL_WM_GrabInput();

  renderer_gl_init(screen_width, screen_height);
}

void renderer_shutdown(void* empty) {
  LOGI("renderer_shutdown");
  renderer_gl_shutdown();
}

void at_exit() {
  SDL_Quit();
}

void signal_render_complete(void* _allocator) {
  renderer_end_frame();
  StackAllocator allocator = (StackAllocator)_allocator;
  SDL_GL_SwapBuffers();
  render_reply_queue->enqueue(allocator);

#ifdef WINDOWS
  // on windows, the input events go to the thread that owns the
  // window so we have to process them here.
  update_input_state();
#endif
}
