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
#include "testlib.h"
#include "testlib_internal.h"
#include "stb_image.h"
#include "utils.h"
#include "input.h"

#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>

ThreadBarrier render_barrier;
FixedAllocator* clock_allocator;
FixedAllocator* image_resource_allocator;
StackAllocator frame_allocator;
CommandQueue* render_queue;
AllocatorQueue* render_reply_queue;

uint32_t screen_width;
uint32_t screen_height;

const char* libbase;

static Thread renderer_thread;

void process_render_command() {
  Command* command = render_queue->dequeue();
  command->function(command->data);
}

static int renderer_running = 0;
void* renderer_exec(void* empty) {
  while(renderer_running) {
    process_render_command();
  }
  return NULL;
}

void renderer_await_startup(void* empty) {
  threadbarrier_wait(render_barrier);
}

void testlib_init() {
  input_init();
  clock_allocator = new FixedAllocator(sizeof(struct Clock_), MAX_NUM_CLOCKS, "clock_allocator");
  image_resource_allocator = new FixedAllocator(sizeof(struct ImageResource_), MAX_NUM_IMAGES, "image_resource_allocator");
  frame_allocator = stack_allocator_make(1024 * 1024, "frame_allocator1");

  render_queue = new CommandQueue();
  render_reply_queue = new AllocatorQueue();

  // get our offscreen allocator primed
  render_reply_queue->enqueue(stack_allocator_make(1024 * 1024, "frame_allocator2"));
  //render_reply_queue->enqueue(stack_allocator_make(1024 * 1024, "frame_allocator3"));

  render_barrier = threadbarrier_make(2);
}

void lib_init(int argc, char** argv) {
  // won't work on android, need to decide what to do here instead
  char* dir = dirname(strdup(argv[0]));
  int nbuffer = strlen(dir) + 32;
  char* buffer = (char*)malloc(nbuffer);
  snprintf(buffer, nbuffer, "%s/../", dir);
  libbase = buffer;


  testlib_init();

  native_init();

  renderer_running = 1;
  renderer_thread = thread_create(renderer_exec, NULL);

  // let the renderer finish init
  renderer_enqueue_sync(renderer_init, NULL);

  // kick off the audio system
  audio_init();
}

void render_loop_exit(void* empty) {
  renderer_running = 0;
  threadbarrier_wait(render_barrier);
}

void lib_shutdown() {
  images_free();
  renderer_enqueue(renderer_shutdown, NULL);
  renderer_enqueue_sync(render_loop_exit, NULL);
  at_exit();
}

void begin_frame() {
  stack_allocator_freeall(frame_allocator);
  renderer_enqueue(renderer_begin_frame, NULL);
}

void* frame_alloc(size_t bytes) {
  return stack_allocator_alloc(frame_allocator, bytes);
}

void end_frame() {
  renderer_enqueue(signal_render_complete, frame_allocator);
  frame_allocator = render_reply_queue->dequeue();
}

static DLL_DECLARE(ImageResource_, node) image_resources;

int image_width(ImageResource resource) {
  return resource->w;
}

int image_height(ImageResource resource) {
  return resource->h;
}

ImageResource image_load(const char * file) {
  int w, h, channels;
  unsigned char *data = stbi_load(file, &w, &h, &channels, 0);

  if(data == NULL) fail_exit("failed to load %s\n", file);

  ImageResource resource = (ImageResource)image_resource_allocator->alloc();
  resource->w = w;
  resource->h = h;
  resource->channels = channels;
  resource->data = data;
  image_resources.add_head(resource);

  resource->texture = new Texture();
  renderer_enqueue(renderer_finish_image_load, resource);

  return resource;
}

void image_free(ImageResource resource) {
  renderer_enqueue(renderer_finish_image_free,
                   resource->texture);
  image_resources.remove(resource);
  image_resource_allocator->free(resource);
}

void images_free() {
  DLLNode node = image_resources.head;
  while(node) {
    DLLNode next = node->next;
    ImageResource resource = image_resources.to_element(node);
    image_free(resource);
    node = next;
  }
}

/* portable implementation */
Clock clock_make() {
  Clock clock = (Clock)clock_allocator->alloc();
  clock->cycles = 0;
  clock->time_scale = 1.0f;
  clock->paused = 0;
  return clock;
}

void clock_free(Clock clock) {
  clock_allocator->free(clock);
}

float clock_update(Clock clock, float delta) {
  if(!clock->paused) {
    float scaled = delta * clock->time_scale;
    clock->cycles += clock_seconds_to_cycles(scaled);
    return scaled;
  } else {
    return 0.0f;
  }
}

long clock_time(Clock clock) {
  return clock->cycles;
}

float clock_cycles_to_seconds(long cycles) {
  return cycles / 1000.0f;
}

long clock_seconds_to_cycles(float seconds) {
  return roundf(seconds * 1000.0f);
}

Sprite frame_make_sprite() {
  Sprite sprite = (Sprite)frame_alloc(sizeof(struct Sprite_));
  sprite->angle = 0.0f;
  sprite->originX = 0.0f;
  sprite->originY = 0.0f;

  sprite->u0 = 0.0f;
  sprite->v0 = 1.0f;
  sprite->u1 = 1.0f;
  sprite->v1 = 0.0f;

  sprite->w = 100;
  sprite->h = 100;
  return sprite;
}

OBJECT_IMPL(Renderable, Object);

Renderable::Renderable(void* empty) {
}

Renderable::~Renderable() {
}

void Renderable::render(void *args) {
}

static void renderable_delete(Renderable* renderable) {
  delete renderable;
}

void Renderable::destroy() {
  // ask the render thread to delete us to make sure that destruction
  // is pipelined properly with any rendering
  renderer_enqueue(renderable_delete, this);
}

extern void renderable_render_to_screen(RenderableCommand* command);

void renderable_enqueue_for_screen(Renderable* renderable, void *args) {
  RenderableCommand* command = (RenderableCommand*)frame_alloc(sizeof(RenderableCommand));
  command->renderable = renderable;
  command->args = args;
  command->next = NULL;
  renderer_enqueue(renderable_render_to_screen, command);
}

extern void renderables_render_to_screen(RenderableCommand* command);

void renderables_enqueue_for_screen(RenderableCommand* commands) {
  renderer_enqueue(renderables_render_to_screen, commands);
}

Command::Command(CommandFunction function, void* data)
  : function(function), data(data) {
  node.next = NULL;
  node.prev = NULL;
}

static Command* command_make(CommandFunction function, void* data) {
  return new (frame_alloc(sizeof(Command))) Command(function, data);
}

void command_async(CommandQueue* queue, CommandFunction function, void* data) {
  Command* command = command_make(function, data);
  queue->enqueue(command);
}

static void command_sync_function(ThreadBarrier b) {
  threadbarrier_wait(b);
}

void command_sync(CommandQueue* queue, ThreadBarrier b,
                  CommandFunction function, void* data) {
  command_async(queue, function, data);
  command_async(queue, (CommandFunction)command_sync_function, b);
  threadbarrier_wait(b);
}
