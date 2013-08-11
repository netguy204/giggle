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
#include "threadlib.h"
#include "audio.h"
#include "memlib.h"
#include "testlib.h"

#include <limits>
#include <new>

const int int16_min = std::numeric_limits<int16_t>::min();
const int int16_max = std::numeric_limits<int16_t>::max();

PlayList* playlist;
CommandQueue* audio_queue;
Filter global_filter;

FixedAllocator* cmd_allocator;

PlayListSample::PlayListSample(Sampler* sampler)
  : sampler(sampler) {
  handle = next_sampler_handle++;
}

PlayListSample::~PlayListSample() {
  delete sampler;
}

void* PlayListSample::operator new(size_t ignored) {
  return PlayListSample::allocator.alloc();
}

void PlayListSample::operator delete(void* obj) {
  PlayListSample::allocator.free(obj);
}

long PlayListSample::next_sampler_handle = 0;
FixedAllocator PlayListSample::allocator(sizeof(PlayListSample),
                                         MAX_NUM_SAMPLERS,
                                         "pls_allocator");

PlayList::PlayList()
  : next_sample(0), nbuffer(4096 * 2) {
  mix_buffer = (long*)malloc(sizeof(long) * nbuffer);
}

PlayList::~PlayList() {
  delete mix_buffer;
}

void PlayList::insert_sampler(PlayListSample* sample) {
  this->samples.insert_before_when(sample, [=](PlayListSample* other) {
      return START(sample->sampler) < START(other->sampler);
    });
}

void PlayList::fill_buffer(int16_t* buffer, int nsamples) {
  int ii;
  long next_sample = this->next_sample;

  // make sure there's room to mix
  this->next_sample += nsamples;
  if(nsamples > nbuffer) {
    nbuffer = nsamples;
    mix_buffer = (long*)realloc(mix_buffer, sizeof(long) * nbuffer);
  }
  memset(mix_buffer, 0, sizeof(long) * nbuffer);

  PlayListSample* node;
  for(node = (PlayListSample*)this->samples.head; node != NULL;
      node = (PlayListSample*)node->node.next) {
    if(START(node->sampler) > this->next_sample) break;

    if(START(node->sampler) > next_sample) {
      // starting midway through the buffer
      long local_offset = START(node->sampler) - next_sample;
      long local_nsamples = nsamples - local_offset;
      node->sampler->sample(&mix_buffer[local_offset], START(node->sampler), local_nsamples);
    } else {
      node->sampler->sample(mix_buffer, next_sample, nsamples);
    }
  }

  /*
  static FILE* hack = fopen("hack.txt", "w");
  for(int ii = 0; ii < nsamples; ++ii) {
    fprintf(hack, "%ld\n", mix_buffer[ii]);
  }
  */

  for(ii = 0; ii < nsamples; ++ii) {
    long value = mix_buffer[ii];

    if(value > int16_max) {
      buffer[ii] = int16_max;
    } else if(value < int16_min) {
      buffer[ii] = int16_min;
    } else {
      buffer[ii] = value;
    }
  }

  /* remove any nodes that are no longer playable */
  node = (PlayListSample*)this->samples.head;
  while(node) {
    PlayListSample* next = (PlayListSample*)node->node.next;
    if(END(((PlayListSample*)node)->sampler) < this->next_sample) {
      this->samples.remove(node);
      delete node;
    }
    node = next;
  }
}

PlayListSample* PlayList::find_sample(long handle) {
  PlayListSample* result = NULL;
  samples.foreach([&](PlayListSample* item) -> int {
      if(item->handle == handle) {
        result = item;
        return 1;
      } else {
        return 0;
      }
    });
  return result;
}

void audio_init() {
  sampler_init();

  cmd_allocator = new FixedAllocator(sizeof(Command), MAX_NUM_SAMPLERS,
                                     "audio_cmd_allocator");

  playlist = new PlayList();
  audio_queue = new CommandQueue();
  global_filter = lowpass_make(0, 0);

  native_audio_init();
}

/* another version of this exists in testlib.cpp but we use a
   different allocator */
static Command* command_make(CommandFunction func, void* data) {
  return new (cmd_allocator->alloc()) Command(func, data);
}

static void command_free(Command* command) {
  cmd_allocator->free(command);
}

static void command_insert_sample(PlayListSample* sample) {
  playlist->insert_sampler(sample);
}

AudioHandle* audio_enqueue(Sampler* sampler) {
  PlayListSample* sample = new PlayListSample(sampler);
  AudioHandle* handle = new AudioHandle(sample);

  Command* command = command_make((CommandFunction)command_insert_sample, sample);
  audio_queue->enqueue(command);
  return handle;
}

long audio_current_sample() {
  /* race condition but we don't care */
  return playlist->next_sample;
}

void audio_fill_buffer(int16_t* buffer, int nsamples) {
  Command* command;
  while((command = audio_queue->dequeue_noblock()) != NULL) {
    command->function(command->data);
    command_free(command);
  }

  playlist->fill_buffer(buffer, nsamples);
}

OBJECT_IMPL(AudioHandle, Object);
OBJECT_PROPERTY(AudioHandle, last_sample);
OBJECT_PROPERTY(AudioHandle, handle);

AudioHandle::AudioHandle(void* _sample) {
  PlayListSample* sample = (PlayListSample*)_sample;
  handle = sample->handle;
  last_sample = END(sample->sampler);
}

AudioHandle::AudioHandle(const AudioHandle& other)
  : handle(other.handle), last_sample(other.last_sample) {
}

int AudioHandle::isCurrent() {
  return audio_current_sample() < last_sample;
}

static void command_terminate(AudioHandle* handle) {
  PlayListSample* sample = playlist->find_sample(handle->handle);
  if(!sample) return;

  playlist->samples.remove(sample);
  delete sample;
}

void AudioHandle::terminate() {
  Command* command = command_make((CommandFunction)command_terminate, this);
  audio_queue->enqueue(command);
}

static void command_destroy(AudioHandle* handle) {
  delete handle;
}

void AudioHandle::destroy() {
  Command* command = command_make((CommandFunction)command_destroy, this);
  audio_queue->enqueue(command);
}
