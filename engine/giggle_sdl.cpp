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

#include <SDL2/SDL.h>

#include "giggle.h"
#include "testlib_internal.h"
#include "gl_headers.h"
#include "utils.h"
#include "input.h"
#include "giggle_gl.h"

#include <math.h>
#include <map>

// remap the SDL keycodes to our internal keycodes
typedef std::map<unsigned long, int> SDLRemapTable;
static SDLRemapTable sdl_remap_table;
static InputState_ pstate;
void update_input_state();

class SDLRenderer : public Renderer {
protected:
  virtual void initializer() {
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
    sdl_remap_table[SDLK_LALT] = K_ALT;
    sdl_remap_table[SDLK_RALT] = K_ALT;
    sdl_remap_table[SDLK_LGUI] = K_ALT;
    sdl_remap_table[SDLK_RGUI] = K_ALT;


    LOGI("renderer_init");
    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 ) {
      fail_exit("Unable to init SDL: %s\n", SDL_GetError());
    }

    /* probably supported in sdl > 1.2
       SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
       SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    */

    screen = SDL_CreateWindow("GGL",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              screen_width, screen_height,
                              SDL_WINDOW_OPENGL);
    SDL_GL_CreateContext(screen);

    if(screen == NULL) {
      fail_exit("Unable to set %dx%d video: %s\n",
                screen_width, screen_height, SDL_GetError());
    }

    GLenum err = glewInit();
    if(err != GLEW_OK) {
      fail_exit("Failed to initialize GLEW\n");
    } else {
      LOGI("glew initialized");
    }

    Renderer::initializer();
  }

  virtual void renderer_begin_frame() {
    Renderer::renderer_begin_frame();
    update_input_state();
  }

  virtual void renderer_end_frame(StackAllocator active_allocator) {
    SDL_GL_SwapWindow(screen);
    Renderer::renderer_end_frame(active_allocator);
  }

public:
  SDL_Window* screen;

  SDLRenderer(Giggle* giggle, int depth, size_t sz)
    : Renderer(giggle, depth, sz) {
  }

  virtual bool step() {
    if(pstate.quit_requested) return false;

    return Renderer::step();
  }

  long time_millis() {
    return SDL_GetTicks();
  }

  void sleep_millis(long millis) {
    SDL_Delay(millis);
  }

  void show_mouse(bool show) {
    SDL_ShowCursor(show);
  }

};

Renderer* sdl_renderer(Giggle* giggle) {
  return new SDLRenderer(giggle, 2, 1024 * 1024);
}

class SDLNTRenderer : public SDLRenderer {
public:
  SDLNTRenderer(Giggle* giggle, int depth, size_t sz)
    : SDLRenderer(giggle, depth, sz) {
  }

  void enqueue(CommandFunction function, void* data) {
    function(data);
  }
};

Renderer* sdl_nonthreaded_renderer(Giggle* giggle) {
  return new SDLNTRenderer(giggle, 1, 1024 * 1024);
}

// internal stuff
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
        SDLRemapTable::const_iterator remapped = sdl_remap_table.find(event.key.keysym.sym);
        if(remapped != sdl_remap_table.end()) {
          handle_key((KeyNumber)remapped->second, keydown * 1.0f);
        }
        break;
      }
    } else if(event.type == SDL_MOUSEBUTTONDOWN) {
      handle_key(mouse2kn(event.button.button), 1.0f);
    } else if(event.type == SDL_MOUSEBUTTONUP) {
      handle_key(mouse2kn(event.button.button), 0.0f);
    } else if(event.type == SDL_MOUSEMOTION) {
      SpatialInput input;
      input.absolute.x = event.motion.x;
      input.absolute.y = (GIGGLE->renderer->screen_height - event.motion.y);
      input.relative.x = event.motion.xrel;
      input.relative.y = -event.motion.yrel;
      handle_spatialinput(SI_MOUSE, &input);
    }
  }
}

void inputstate_latest(InputState state) {
  *state = pstate;
}
