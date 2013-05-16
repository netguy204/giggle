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
#ifndef SAMPLER_H
#define SAMPLER

#include <ivorbisfile.h>

#include "config.h"

#include <stdint.h>

#define N_(n,b) (n*b)

#define C_(n) N_(n, 261.6)
#define D_(n) N_(n, 293.7)
#define E_(n) N_(n, 329.6)
#define F_(n) N_(n, 349.2)
#define G_(n) N_(n, 392.0)
#define A_(n) N_(n, 440.0)
#define B_(n) N_(n, 493.9)

typedef int16_t (*SamplerFunction)(void*, long);
typedef void (*ReleaseSampler)(void*);

void sampler_init();

class Sampler {
 public:
  virtual void sample(long* result, long start, long nsamples) = 0;
  virtual ~Sampler();

  long start_sample;
  long duration_samples;
};

class BaseSampler : public Sampler {
 public:
  virtual void sample(long* result, long start, long nsamples) = 0;
  virtual ~BaseSampler();
  void* operator new(size_t sz);
  void operator delete(void*);
};

class SinSampler : public BaseSampler {
 public:
  SinSampler(long start, long duration, float freq,
             float amp, float phase);
  virtual ~SinSampler();

  virtual void sample(long* result, long start, long nsamples);

  float phase; /* radians */
  float radians_per_sample;
  float amp;
};

class SawSampler : public BaseSampler {
 public:
  SawSampler(long start, long duration,
             float freq, float amp, float phase);
  virtual ~SawSampler();

  virtual void sample(long* result, long start, long nsamples);

  long samples_per_period;
  long phase_samples;
  float slope;
  float amp;
};

class OggSampler : public Sampler {
 public:
  OggSampler(const char* filename, long sample, float volume);
  virtual ~OggSampler();

  virtual void sample(long* result, long start, long nsamples);

  FILE* f;
  OggVorbis_File vf;
  long sample_rate;
  long samples_past;
  float volume;
  int channels;
  char buffer[8192];
};

class BufferSampler : public Sampler {
 public:
  BufferSampler(int16_t* buffer, long start, long nsamples);
  virtual ~BufferSampler();

  virtual void sample(long* result, long start, long nsamples);

  int16_t* buffer;
};

class LinearAmplitudeSampler : public Sampler {
 public:
  LinearAmplitudeSampler(Sampler* basis, long begin, float begin_value,
                         float slope, long duration_samples);

  virtual ~LinearAmplitudeSampler();

  virtual void sample(long* result, long start, long nsamples);

  Sampler* basis;
  long begin;
  float begin_value;
  float slope;
};

#define DURATION(f) ((f)->duration_samples)
#define START(f) ((f)->start_sample)
#define END(f) (START(f) + DURATION(f))

typedef struct Filter_ {
  int na;
  int nb;
  int xi;
  float yi;

  float *as;
  float *bs;
  int16_t *xs;
  int16_t *ys;
} *Filter;

Filter filter_make(float* as, int na, float* bs, int nb);

int16_t filter_value(Filter filter, int16_t value);

Filter lowpass_make(float cutoff, float sample_freq);

#endif
