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
 *
 * Functions that deal with objects are prefixed with objectname_
 */

#include "threadlib.h"
#include "memlib.h"
#include "listlib.h"
#include "rect.h"
#include "audio.h"
#include "config.h"
#include "texture.h"
#include "random.h"
#include "ooc.h"

#include <stdint.h>

extern uint32_t screen_width;
extern uint32_t screen_height;

extern const char* libbase;

/* initialize the internal allocators for the library. Must be called
   before other functions */
void testlib_init(); // called by lib_init
void lib_init(int argc, char** argv);

/* shuts down the renderer and frees memory. Must be called before
   termination */
void lib_shutdown();

/* must be called from render thread */
void process_render_command();
void renderer_resize(int w, int h);

void begin_frame();
void* frame_alloc(size_t bytes);
void end_frame();

typedef struct InputState_ {
  int quit_requested;
} *InputState;

typedef void(*GameStep)(long delta);
void set_game_step(GameStep fn);

/* provided by system specific lib */
void native_init();
void inputstate_latest(InputState state);
long time_millis();
void sleep_millis(long millis);
void renderer_gl_init(int w, int h);
void renderer_gl_shutdown();
void window_show_mouse(bool show);

/* exported by realmain */
int loop_once();

/* exported by testlib.c */
typedef struct Clock_ {
  long cycles; /* msecs */
  float time_scale;
  int paused;
} *Clock;

Clock clock_make();
void clock_free(Clock clock);
float clock_update(Clock clock, float delta); /* time in seconds */
long clock_time(Clock clock); /* time in cycles */

float clock_cycles_to_seconds(long cycles);
long clock_seconds_to_cycles(float seconds);

typedef struct ImageResource_ {
  struct DLLNode_ node;
  int w, h;
  Texture* texture;
  int channels;
  unsigned char* data; /* shortlived, internal */
} *ImageResource;

ImageResource image_load(const char * file);
int image_width(ImageResource resource);
int image_height(ImageResource resource);
void image_free(ImageResource resource);
void images_free();

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

Sprite frame_make_sprite();

typedef struct ColoredRect_ : Rect_ {
  ColoredRect_* next;
  float color[4];
} *ColoredRect;

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

extern Renderable* basespritelist_renderer;
extern Renderable* spritelist_renderer;
extern Renderable* coloredspritelist_renderer;
extern Renderable* rect_renderer;
extern Renderable* filledrect_renderer;

struct RenderableCommand {
  Renderable* renderable;
  RenderableCommand* next;
  void* args;
};

void renderable_enqueue_for_screen(Renderable* renderable, void* arg);
void renderables_enqueue_for_screen(RenderableCommand* commands);

typedef void (*CommandFunction)(void*);

class Command {
public:
  Command(CommandFunction function, void* data);

  struct DLLNode_ node;
  CommandFunction function;
  void *data;
};

typedef Queue<StackAllocator_, offsetof(StackAllocator_, node)> AllocatorQueue;
extern AllocatorQueue* render_reply_queue;

typedef Queue<Command, offsetof(Command, node)> CommandQueue;

void command_async(CommandQueue* queue, CommandFunction function, void* data);
void command_sync(CommandQueue* queue, ThreadBarrier b,
                  CommandFunction function, void* data);

#define renderer_enqueue(function, data) \
command_async(render_queue, (CommandFunction)function, (void*)data)

#define renderer_enqueue_sync(function, data) \
  command_sync(render_queue, render_barrier, \
               (CommandFunction)function, (void*)data)

#endif
