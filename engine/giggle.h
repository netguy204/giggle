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
#ifndef TEST_H
#define TEST_H

/** conventions:
 *
 * Things that happen on the renderer thread are prefixed with
 * renderer_
 *
 * Things that allocate from the per frame memory allocator are
 * prefixed with frame_
 */

#include "threadlib.h"
#include "memlib.h"
#include "listlib.h"
#include "rect.h"
#include "config.h"
#include "texture.h"
#include "random.h"
#include "task.h"
#include "matrix.h"
#include "ooc.h"
#include "gl_headers.h"

#include <stdint.h>

typedef void (*CommandFunction)(void*);

typedef struct ImageResource_ {
  struct DLLNode_ node;
  int w, h;
  Texture* texture;
  int channels;
  unsigned char* data; /* shortlived, internal */
} *ImageResource;

typedef struct BaseSprite_ {
  BaseSprite_* next;
  Texture* texture;
  unsigned int count;
  float u0, u1, v0, v1;
  int16_t displayX;
  int16_t displayY;
  int16_t w, h;
} *BaseSprite;

#define sprite_append(list, sprite) do {        \
    sprite->next = list;                       \
    if(list) {                                  \
      sprite->count = (list)->count + 1;        \
    } else {                                    \
      sprite->count = 1;                        \
    }                                           \
    list = sprite;                              \
  } while(0)

typedef struct Sprite_ : BaseSprite_ {
  float angle;
  float originX;
  float originY;
  float color[4];
} *Sprite;

// this is the last resort rendering mechanism for very special cases
class Renderable : public Object {
 public:
  OBJECT_PROTO(Renderable);
  Renderable(void *blank);

  virtual ~Renderable();

  // constructor will be called on the game thread, memory is owned by
  // the game thread, destruction happens on the render thread so that
  // destruction and rendering are coordinated
  void destroy();

  // render will be called on the render thread
  virtual void render(void* args);
};

struct RenderableCommand {
  Renderable* renderable;
  RenderableCommand* next;
  void* args;
};

class Command {
public:
  Command(CommandFunction function, void* data);

  struct DLLNode_ node;
  CommandFunction function;
  void *data;
};

typedef Queue<StackAllocator_, offsetof(StackAllocator_, node)> AllocatorQueue;
typedef Queue<Command, offsetof(Command, node)> CommandQueue;

class Clock {
public:
  long cycles; /* msecs */
  float time_scale;
  int paused;

  Clock();
  float update(float delta);
  long time();

  float cycles_to_seconds(long cycles);
  long seconds_to_cycles(float seconds);
};

class Giggle;

class GLMemory {
 public:
  GLMemory();

  void* data;
  size_t size;
  GLenum access;
  GLuint buffer;
};

class Renderer : public Task {
private:
  ThreadBarrier render_barrier;
  StackAllocator frame_allocator;
  StackAllocator renderer_allocator;

  CommandQueue render_queue;
  AllocatorQueue render_reply_queue;
  bool step_complete;

  static void _renderer_mark_step_complete(void* _renderer);
  static void _renderer_begin_frame(void* _renderer);
  static void _renderer_end_frame(void* _renderer);
  static void _renderer_renderables(void* _commands);

protected:
  Giggle* giggle;

  virtual void initializer();

  // called from the render thread. If overridden, make sure to call
  // the default implementation as well for memory management to work
  // correctly
  virtual void renderer_begin_frame();
  virtual void renderer_end_frame(StackAllocator active_allocator);

public:
  uint32_t screen_width;
  uint32_t screen_height;

  Renderable* basespritelist_renderer;
  Renderable* spritelist_renderer;
  Renderable* coloredspritelist_renderer;
  Renderable* rect_renderer;
  Renderable* filledrect_renderer;

  Matrix44 orthographic_projection;

  Renderer(Giggle* giggle, int depth, size_t sz);

  virtual bool step();

  virtual void begin_frame();
  void enqueue(Renderable* renderable, void* arg);
  void enqueue(RenderableCommand* commands);
  virtual void enqueue(CommandFunction function, void* data);
  virtual void end_frame();

  // only for use from the render thread (if there is one)
  virtual GLMemory* gl_bufinit(GLMemory* mem);
  virtual GLMemory* gl_claim(GLMemory* mem, size_t sz);
  virtual void gl_unclaim(GLMemory* mem);


  // in/for are independently thread safe but not internally thread
  // safe
  virtual void* renderer_alloc(size_t sz);
  virtual void* alloc(size_t sz);

  // no portable base implementation is available for these methods so
  // foundation library must supply
  virtual long time_millis() = 0;
  virtual void sleep_millis(long millis) = 0;
  virtual void show_mouse(bool show) = 0;
};

class GameLogic : public Task {
protected:
  Giggle* giggle;

  virtual void initializer() = 0;

public:
  GameLogic(Giggle* giggle);

  virtual bool step() = 0;
};

class AudioHandle : public Object {
 public:
  OBJECT_PROTO(AudioHandle);
  AudioHandle(void* _);
  AudioHandle(const AudioHandle& other);

  virtual int isCurrent();
  virtual void terminate();

  long handle;
  long last_sample;
};

class Sound {
 public:
  virtual ~Sound();
};

class AudioSystem : public Task {
protected:
  typedef std::map<long, AudioHandle*> LongToHandle;
  Mutex mutex;

  Giggle* giggle;
  LongToHandle long_to_handle;

  virtual void initializer() = 0;

public:
  AudioSystem(Giggle* giggle);

  virtual Sound* get_sound(const char* name, float scale) = 0;
  virtual AudioHandle* play_sound(Sound* sound, int channel) = 0;
  virtual AudioHandle* stream_sound(const char* fname, long start) = 0;
  virtual long current_sample() const = 0;

  virtual AudioHandle* sound_handle(long handle);
  virtual bool step();
};


class Giggle {
public:
  Renderer* renderer;
  GameLogic* logic;
  AudioSystem* audio;

  FixedAllocator* clock_allocator;
  FixedAllocator* image_resource_allocator;
  DLL_DECLARE(ImageResource_, node) image_resources;

  Giggle();

  ImageResource image_load(const char* file);
  void image_free(ImageResource resource);
  void images_free();

  Clock* clock_make();
  void clock_free(Clock* clock);

  Sprite make_sprite();
};

extern Giggle* GIGGLE;
Giggle* lib_init(int argc, char** argv);

typedef struct InputState_ {
  int quit_requested;
} *InputState;

/* provided by system specific lib */
void inputstate_latest(InputState state);


/* Giggle is very much married to OpenGL and there's no sense in
   pretending otherwise. */
#include "giggle_gl.h"

#endif
