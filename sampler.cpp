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
#include "sampler.h"
#include "memlib.h"
#include "config.h"
#include "utils.h"

#include <math.h>
#include <stdlib.h>
#include <memory.h>

FixedAllocator* sampler_allocator;

#define SIN_TABLE

#ifdef SIN_TABLE
#define TABLE_SIZE 1024
float *sin_table;
#define SIN(x) sin_table[((int)((x * TABLE_SIZE) / (2*M_PI))) % TABLE_SIZE]
#else
#define SIN(x) sinf(x)
#endif

void sampler_init() {
#ifdef SIN_TABLE
  int ii;
  sin_table = (float*)malloc(sizeof(float) * TABLE_SIZE);

  for(ii = 0; ii < TABLE_SIZE; ++ii) {
    float factor = (float)ii/TABLE_SIZE;
    sin_table[ii] = sin(2 * M_PI * factor);
  }
#endif

  size_t max_sampler_size
    = MAX(sizeof(SinSampler),
          MAX(sizeof(SawSampler),
              sizeof(struct Filter_)));

  sampler_allocator = new FixedAllocator(max_sampler_size,
                                         MAX_NUM_SAMPLERS,
                                         "sampler_allocator");
}

void sampler_free(void* obj) {
  sampler_allocator->free(obj);
}

long sampler_offset(Sampler* sampler, long sample) {
  return sample - START(sampler);
}

Sampler::~Sampler() {}

BaseSampler::~BaseSampler() {}

void* BaseSampler::operator new(size_t sz) {
  return sampler_allocator->alloc();
}

void BaseSampler::operator delete(void* v) {
  sampler_allocator->free(v);
}

SinSampler::SinSampler(long start, long duration, float freq,
                       float amp, float phase) {
  start_sample = start;
  duration_samples = duration;

  float cycles_per_sample = freq / SAMPLE_FREQ;
  radians_per_sample = cycles_per_sample * 2 * M_PI;
  this->amp = amp;
  this->phase = phase * 2 * M_PI;
}

SinSampler::~SinSampler() {}

void SinSampler::sample(long* result, long sample, long nsamples) {
  long max_sample = MIN(END(this), sample + nsamples);
  nsamples = max_sample - sample;

  sample = sampler_offset(this, sample);

  for(int ii = 0; ii < nsamples; ++ii) {
    result[ii] += amp * SIN(phase + radians_per_sample * (sample + ii));
  }
}

/** all phases are normalized phase values ranging from [0, 1)
 */


void SawSampler::sample(long* result, long sample, long nsamples) {
  long max_sample = MIN(END(this), sample + nsamples);
  nsamples = max_sample - sample;

  sample = sampler_offset(this, sample);

  for(int ii = 0; ii < nsamples; ++ii) {
    long pos = (phase_samples + sample + ii) % samples_per_period;
    result[ii] += -amp + pos * slope;
  }
}

SawSampler::SawSampler(long start, long duration,
                       float freq, float amp, float phase) {
  this->start_sample = start;
  this->duration_samples = duration;

  long samples_per_period = SAMPLE_FREQ / freq;
  float slope = 2 * amp / samples_per_period;

  this->samples_per_period = samples_per_period;
  this->slope = slope;
  this->phase_samples = phase * samples_per_period;
  this->amp = amp;
}

void oggsampler_fillbuffer(OggSampler* sampler) {
  int section;
  int read = 0;
  while(read < sizeof(sampler->buffer)) {
    int ret = ov_read(&sampler->vf, &sampler->buffer[read], sizeof(sampler->buffer) - read, &section);
    if(ret == 0) {
      // done
      memset(&sampler->buffer[read], 0, sizeof(sampler->buffer) - read);
      read = sizeof(sampler->buffer);
    } else if(ret < 0) {
      if(ret == OV_EBADLINK) {
        LOGW("corrupt bitsream!");
        exit(1);
      }
      LOGW("unknown error");
    }
    read += ret;
  }

  /*
  static FILE* hack = fopen("hack2.txt", "w");
  int16_t* buffer = (int16_t*)sampler->buffer;
  size_t count = sizeof(sampler->buffer)/sizeof(int16_t);
  for(int ii = 0; ii < count; ++ii) {
    fprintf(hack, "%d\n", buffer[ii]);
  }
  */
}

SawSampler::~SawSampler() {}

OggSampler::OggSampler(const char* filename, long start, float volume) {
  f = fopen(filename, "rb");
  if(ov_open(f, &vf, NULL, 0) < 0) {
    LOGW("error opening vorbis file %s\n", filename);
    exit(1);
  }

  vorbis_info *vi = ov_info(&vf, -1);
  start_sample = start;
  duration_samples = ov_pcm_total(&vf, -1) * vi->channels;
  samples_past = 0;
  channels = vi->channels;
  sample_rate = vi->rate;
  this->volume = volume;
  oggsampler_fillbuffer(this);
}

OggSampler::~OggSampler() {
  ov_clear(&vf);
}

void OggSampler::sample(long* result, long sample, long nsamples) {
  const int bufsz = sizeof(buffer) / (sizeof(int16_t));
  int16_t* buffer = (int16_t*)this->buffer;

  long max_sample = MIN(END(this), sample + nsamples);
  nsamples = max_sample - sample;
  sample = sampler_offset(this, sample);

  // make sure first sample is past the end of our buffer
  while(sample - samples_past >= bufsz) {
    oggsampler_fillbuffer(this);
    samples_past += bufsz;
  }

  int ridx = 0;
  while(ridx < nsamples) {
    int loop_samples = MIN(bufsz, nsamples - ridx);
    for(int ii = 0; ii < loop_samples; ++ii) {
      result[ridx++] += buffer[ii] * volume;
    }
    if(ridx < nsamples) {
      oggsampler_fillbuffer(this);
      samples_past += bufsz;
    }
  }
}


void BufferSampler::sample(long* result, long sample, long nsamples) {
  long max_sample = MIN(END(this), sample + nsamples);
  nsamples = max_sample - sample;
  sample = sampler_offset(this, sample);

  for(int ii = 0; ii < nsamples; ++ii) {
    result[ii] += buffer[sample + ii];
  }
}

BufferSampler::~BufferSampler() {}

BufferSampler::BufferSampler(int16_t* buffer, long start, long nsamples) {
  start_sample = start;
  duration_samples = nsamples;
  this->buffer = buffer;
}

LinearAmplitudeSampler::LinearAmplitudeSampler(Sampler* basis, long begin, float begin_value,
                                               float slope, long duration_samples)
  : basis(basis), begin(begin), begin_value(begin_value), slope(slope) {
  this->duration_samples = duration_samples;
}

LinearAmplitudeSampler::~LinearAmplitudeSampler() {
  delete basis;
}

void LinearAmplitudeSampler::sample(long* result, long start, long nsamples) {
  basis->sample(result, start, nsamples);

  long x0 = start - begin;
  for(int ii = 0; ii < nsamples; ++ii) {
    long x = x0 + ii;
    if(x < 0) continue;

    float amp = x * slope + begin_value;
    result[ii] *= amp;
  }
}

int16_t filter_value(Filter filter, int16_t value) {
  int ii;
  int xi = filter->xi;
  int yi = filter->yi;

  filter->xi = (xi + 1) % filter->na;
  filter->yi = (yi + 1) % filter->nb;

  filter->xs[xi] = value;

  int16_t result = 0;
  for(ii = 0; ii < filter->na; ++ii) {
    int idx = (xi + ii + 1) % filter->na;
    result += filter->xs[idx] * filter->as[filter->na - ii - 1];
  }

  for(ii = 0; ii < filter->nb; ++ii) {
    int idx = (yi + ii + 1) % filter->nb;
    result += filter->ys[idx] * filter->bs[filter->nb - ii - 1];
  }

  filter->ys[yi] = result;
  return result;
}

void filter_release(Filter filter) {
  free(filter->as);
  free(filter->bs);
  free(filter->xs);
  free(filter->ys);

  sampler_free(filter);
}

Filter filter_make(float* as, int na, float* bs, int nb) {
  Filter filter = (Filter)sampler_allocator->alloc();
  filter->na = na;
  filter->nb = nb;
  filter->xi = 0;
  filter->yi = 0;
  filter->as = (float*)malloc(na * sizeof(float));
  filter->xs = (int16_t*)malloc(na * sizeof(int16_t));
  filter->bs = (float*)malloc(nb * sizeof(float));
  filter->ys = (int16_t*)malloc(nb * sizeof(int16_t));

  memcpy(filter->as, as, na * sizeof(float));
  memcpy(filter->bs, bs, nb * sizeof(float));
  memset(filter->xs, 0, na * sizeof(int16_t));
  memset(filter->ys, 0, nb * sizeof(int16_t));
  return filter;
}

Filter lowpass_make(float cutoff, float sample_freq) {
  /*
  float omega_c = tan(cutoff / sample_freq * M_PI);
  float a = (1 - omega_c) / 2;
  float as[] = {a, a};
  float bs[] = {omega_c};
  */
  float as[] = {0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1};
  float bs[] = {0.0};
  return filter_make(as, array_size(as), bs, array_size(bs));
}
