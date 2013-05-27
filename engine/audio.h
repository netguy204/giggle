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
#ifndef AUDIO_H
#define AUDIO_H

#include "sampler.h"
#include "listlib.h"
#include "ooc.h"
#include "memlib.h"

class PlayListSample {
 public:
  PlayListSample(Sampler* sampler);
  ~PlayListSample();

  struct DLLNode_ node;
  Sampler* sampler;

  void* operator new(size_t ignored);
  void operator delete(void* obj);

  long handle;

  static long next_sampler_handle;
  static FixedAllocator allocator;
};

class PlayList {
 public:
  PlayList();
  ~PlayList();

  DLL_DECLARE(PlayListSample, node) samples;
  long next_sample;

  void insert_sampler(PlayListSample* sample);
  void fill_buffer(int16_t* buffer, int nsamples);
  PlayListSample* find_sample(long handle);

  long nbuffer;
  long* mix_buffer;
};

/* high level api */
void audio_init();
long audio_current_sample();

class AudioHandle : public Object {
 public:
  OBJECT_PROTO(AudioHandle);
  AudioHandle(void* sample);
  AudioHandle(const AudioHandle& other);

  int isCurrent();

  void terminate();
  virtual void destroy();

  long handle;
  long last_sample;
};

AudioHandle* audio_enqueue(Sampler* sampler);
void audio_fill_buffer(int16_t* buffer, int nsamples);

/* provided by the system specific library */
void native_audio_init();

#endif
