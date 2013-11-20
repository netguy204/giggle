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
#include "utils.h"
#include "input.h"
#include "game.h"

// these headers doesn't really exist but emscripten keys off seeing
// them to do its translation magic
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include "emscripten.h"


InputState_ pstate;
void inputstate_latest(InputState state) {
  *state = pstate;
}

class WebRenderer : public Renderer {
protected:
  int sdl_remap_table[SDLK_LAST];

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
        SpatialInput input;
        input.absolute.x = event.motion.x;
        input.absolute.y = (GIGGLE->renderer->screen_height - event.motion.y);
        input.relative.x = event.motion.xrel;
        input.relative.y = -event.motion.yrel;
        handle_spatialinput(SI_MOUSE, &input);
      }
    }
  }

  virtual void initializer() {
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

    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0 ) {
      fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
      exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Surface* screen = SDL_SetVideoMode(screen_width, screen_height,
                                           16, SDL_OPENGL);
    if(screen == NULL) {
      fprintf(stderr, "Unable to set %dx%d video: %s\n",
              screen_width, screen_height, SDL_GetError());
      exit(1);
    }

    Renderer::initializer();
  }

  virtual void renderer_begin_frame() {
    Renderer::renderer_begin_frame();
    update_input_state();
  }

  virtual void renderer_end_frame(StackAllocator active_allocator) {
    SDL_GL_SwapBuffers();
    Renderer::renderer_end_frame(active_allocator);
  }

public:
  SDL_Window* screen;

  WebRenderer(Giggle* giggle, size_t sz)
    : Renderer(giggle, 1, sz) {
  }

  virtual bool step() {
    if(pstate.quit_requested) return false;

    return Renderer::step();
  }

  virtual GLMemory* gl_claim(GLMemory* mem, size_t sz) {
    mem->size = sz;
    mem->data = GIGGLE->renderer->renderer_alloc(sz);
    return mem;
  }

  virtual void gl_unclaim(GLMemory* mem) {
    if(mem->data) {
      gl_check(glBindBuffer(GL_ARRAY_BUFFER, mem->buffer));
      gl_check(glBufferData(GL_ARRAY_BUFFER, mem->size, mem->data, GL_DYNAMIC_DRAW));
      mem->data = NULL;
    }
  }

  virtual long time_millis() {
    return SDL_GetTicks();
  }

  virtual void sleep_millis(long millis) {
    SDL_Delay(millis);
  }

  virtual void show_mouse(bool show) {
    SDL_ShowCursor(show);
  }

  virtual void enqueue(CommandFunction function, void* data) {
    function(data);
  }
};

class WebSound : public Sound {
public:
  Mix_Chunk* sound;

  WebSound(Mix_Chunk* sound)
    : sound(sound) {
  }
};

class WebAudioHandle : public AudioHandle {
public:
  static int next_handle;

  OBJECT_PROTO(WebAudioHandle);

  WebAudioHandle(void* _)
    : AudioHandle(_) {
  }

  WebAudioHandle(long lsample)
    : AudioHandle(NULL) {
    handle = next_handle++;
    last_sample = lsample;
  }

  WebAudioHandle(const WebAudioHandle& other)
    : AudioHandle(other) {
  }

  virtual void terminate() {
    // stop the playback
  }
};

OBJECT_IMPL(WebAudioHandle, AudioHandle);

int WebAudioHandle::next_handle = 0;

class WebAudioSystem : public AudioSystem {
private:
  long last_millis;
  long elapsed_millis;

protected:
  virtual void initializer() {
    if(Mix_OpenAudio(0, 0, 0, 0) != 0) {
      fail_exit("failed to initialize audio");
    }

    last_millis = GIGGLE->renderer->time_millis();
  }

public:
  WebAudioSystem(Giggle* giggle)
    : AudioSystem(giggle), elapsed_millis(0) {
  }

  virtual bool step() {
    long next_millis = GIGGLE->renderer->time_millis();
    elapsed_millis += (next_millis - last_millis);
    last_millis = next_millis;

    return AudioSystem::step();
  }

  virtual Sound* get_sound(const char* name, float scale) {
    return new WebSound(Mix_LoadWAV(name));
  }

  virtual AudioHandle* play_sound(Sound* sound, int channel) {
    WebSound* wsound = (WebSound*)sound;
    Mix_PlayChannel(1, wsound->sound, 0);
    return new WebAudioHandle(current_sample() + wsound->sound->alen);
  }

  virtual AudioHandle* stream_sound(const char* fname, long start) {
    Mix_Music* music = Mix_LoadMUS(fname);
    Mix_PlayMusic(music, 1);
    return new WebAudioHandle(current_sample() + 100 * 44100);
  }

  virtual long current_sample() const {
    return ((double)elapsed_millis / 1000.0) * 44100;
  }
};

// required by things that read files
FILE* nativeOpen(const char* fname) {
  return fopen(fname, "rb");
}

void one_step() {
  GIGGLE->logic->step();
}

int main(int argc, char ** argv) {
  Giggle* giggle = lib_init(argc, argv);
  giggle->renderer = new WebRenderer(giggle, 1024*1024);
  giggle->logic = default_gamelogic(giggle);
  giggle->audio = new WebAudioSystem(giggle);

  giggle->renderer->initialize();
  giggle->logic->initialize();
  giggle->audio->initialize();

  emscripten_set_main_loop(one_step, 0, false);

  return 0;
}
