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
#include "giggle.h"
#include "testlib_internal.h"
#include "stb_image.h"
#include "utils.h"
#include "input.h"
#include "task.h"

#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>

void Renderer::_renderer_mark_step_complete(void* _renderer) {
  Renderer* renderer = (Renderer*)_renderer;
  renderer->step_complete = true;
}

void Renderer::_renderer_begin_frame(void* _renderer) {
  ((Renderer*)_renderer)->renderer_begin_frame();
}

struct EndFrameData {
  Renderer* renderer;
  StackAllocator allocator;
};

void Renderer::_renderer_end_frame(void* _efd) {
  EndFrameData* data = (EndFrameData*)_efd;
  _renderer_mark_step_complete(data->renderer);
  data->renderer->renderer_end_frame(data->allocator);
}

void Renderer::_renderer_renderables(void* _commands) {
  RenderableCommand* commands = (RenderableCommand*)_commands;
  while(commands) {
    commands->renderable->render(commands->args);
    commands = commands->next;
  }
}

Renderer::Renderer(Giggle* giggle, int depth, size_t sz)
  : render_barrier(2), frame_allocator(NULL), renderer_allocator(NULL),
    giggle(giggle), screen_width(1280), screen_height(720),
    basespritelist_renderer(NULL), spritelist_renderer(NULL),
    coloredspritelist_renderer(NULL), rect_renderer(NULL),
    filledrect_renderer(NULL), orthographic_projection(NULL) {
  renderer_allocator = stack_allocator_make(sz, "frame_allocator0");
  for(int ii = 0; ii < depth; ++ii) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "frame_allocator%d", ii + 1);
    render_reply_queue.enqueue(stack_allocator_make(sz, strdup(buffer)));
  }
}

// Renderer::renderer_* are located in giggle_gl.cpp

bool Renderer::step() {
  step_complete = false;
  while(!step_complete) {
    Command* command = render_queue.dequeue();
    command->function(command->data);
  }
  return true;
}

void Renderer::begin_frame() {
  if(frame_allocator) fail_exit("end_frame not called for prev frame");
  frame_allocator = render_reply_queue.dequeue();
  stack_allocator_freeall(frame_allocator);
  enqueue(&Renderer::_renderer_begin_frame, this);
}

void Renderer::enqueue(Renderable* renderable, void* args) {
  RenderableCommand* command = (RenderableCommand*)alloc(sizeof(RenderableCommand));
  command->renderable = renderable;
  command->args = args;
  command->next = NULL;
  enqueue(command);
}

void Renderer::enqueue(RenderableCommand* commands) {
  enqueue(&Renderer::_renderer_renderables, commands);
}

void Renderer::enqueue(CommandFunction function, void* data) {
  Command* command = (Command*)alloc(sizeof(Command));
  new(command) Command(function, data);
  render_queue.enqueue(command);
}

void Renderer::end_frame() {
  if(!frame_allocator) fail_exit("begin_frame not called for current frame");

  EndFrameData* efd = (EndFrameData*)alloc(sizeof(EndFrameData));
  efd->renderer = this;
  efd->allocator = frame_allocator;

  enqueue(&Renderer::_renderer_end_frame, efd);
  frame_allocator = NULL;
}

void* Renderer::renderer_alloc(size_t sz) {
  return stack_allocator_alloc(renderer_allocator, sz);
}

void* Renderer::alloc(size_t sz) {
  return stack_allocator_alloc(frame_allocator, sz);
}

GameLogic::GameLogic(Giggle* giggle)
  : giggle(giggle) {
}

Clock::Clock()
  : cycles(0), time_scale(1.0), paused(0) {
}

float Clock::update(float delta) {
  if(!paused) {
    float scaled = delta * time_scale;
    cycles += seconds_to_cycles(scaled);
    return scaled;
  } else {
    return 0.0f;
  }
}

long Clock::time() {
  return cycles;
}

float Clock::cycles_to_seconds(long cycles) {
  return cycles / 1000.0f;
}

long Clock::seconds_to_cycles(float seconds) {
  return roundf(seconds * 1000.0f);
}

Sound::~Sound() {
}

OBJECT_IMPL(AudioHandle, Object);
OBJECT_PROPERTY(AudioHandle, handle);
OBJECT_PROPERTY(AudioHandle, last_sample);

AudioHandle::AudioHandle(void* _)
  : handle(0), last_sample(0) {
}

AudioHandle::AudioHandle(const AudioHandle& other)
  : handle(other.handle) {
}

int AudioHandle::isCurrent() {
  return 1;
}

void AudioHandle::terminate() {
}

AudioSystem::AudioSystem(Giggle* giggle)
  : giggle(giggle) {
  mutex = mutex_create();
}

AudioHandle* AudioSystem::sound_handle(long handle) {
  AudioHandle* result;
  mutex_lock(mutex);
  LongToHandle::const_iterator iter = long_to_handle.find(handle);
  if(iter == long_to_handle.end()) {
    result = NULL;
  } else {
    result = iter->second;
  }
  mutex_unlock(mutex);
  return result;
}

bool AudioSystem::step() {
  // free any expired handles
  mutex_lock(mutex);
  LongToHandle::iterator iter = long_to_handle.begin();
  while(iter != long_to_handle.end()) {
    long handle_name = iter->first;
    AudioHandle* handle = iter->second;
    ++iter;

    if(!handle->isCurrent()) {
      handle->release();
      long_to_handle.erase(handle_name);
    }
  }
  mutex_unlock(mutex);
  return true;
}

Giggle::Giggle() {
  clock_allocator = new FixedAllocator(sizeof(Clock), MAX_NUM_CLOCKS, "clock");
  image_resource_allocator = new FixedAllocator(sizeof(ImageResource_), MAX_NUM_IMAGES, "image_resource");
}

ImageResource Giggle::image_load(const char * file) {
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
  renderer->enqueue((CommandFunction)renderer_finish_image_load, resource);

  return resource;
}

void Giggle::image_free(ImageResource resource) {
  renderer->enqueue((CommandFunction)renderer_finish_image_free, resource->texture);
  image_resources.remove(resource);
  image_resource_allocator->free(resource);
}

void Giggle::images_free() {
  DLLNode node = image_resources.head;
  while(node) {
    DLLNode next = node->next;
    ImageResource resource = image_resources.to_element(node);
    image_free(resource);
    node = next;
  }
}

Clock* Giggle::clock_make() {
  Clock* clock = (Clock*)clock_allocator->alloc();
  new(clock) Clock();
  return clock;
}

void Giggle::clock_free(Clock* clock) {
  clock_allocator->free(clock);
}

Sprite Giggle::make_sprite() {
  Sprite sprite = (Sprite)renderer->alloc(sizeof(struct Sprite_));
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

Giggle* GIGGLE;

Giggle* lib_init(int argc, char** argv) {
  GIGGLE = new Giggle();

  input_init();

  return GIGGLE;
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
  GIGGLE->renderer->enqueue((CommandFunction)renderable_delete, this);
}

Command::Command(CommandFunction function, void* data)
  : function(function), data(data) {
  node.next = NULL;
  node.prev = NULL;
}
