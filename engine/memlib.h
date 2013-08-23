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
#ifndef MEMORY_H
#define MEMORY_H

#include "listlib.h"
#include "config.h"
#include "threadlib.h"

#include <stdlib.h>

class FixedAllocator {
 public:
  FixedAllocator(size_t obj_size, unsigned int n, const char* name);

  void* alloc();
  void free(void* mem);

#ifdef DEBUG_MEMORY
  const char* name;
  long inflight;
  long max_inflight;
#endif

  Mutex mutex;
  size_t allocation_size;
  void* first_free;
};

typedef struct StackAllocator_ {
#ifdef DEBUG_MEMORY
  const char* name;
  long max_alloced;
#endif

  // so these can be enqueued
  DLLNode_ node;

  void* stack_top;
  void* stack_bottom;
  void* stack_max;
} *StackAllocator;

StackAllocator stack_allocator_make(size_t stack_size,
                                    const char* name);
void* stack_allocator_alloc(StackAllocator allocator, size_t size);
void stack_allocator_freeall(StackAllocator allocator);
void stack_allocator_release(StackAllocator allocator);

typedef struct CircularBuffer_ {
  int read_index;
  int write_index;
  int size;
  int filled;
  char* data;
} *CircularBuffer;

CircularBuffer circularbuffer_make(size_t bytes);
void circularbuffer_free(CircularBuffer buffer);
int circularbuffer_bytes_writable(CircularBuffer buffer);
int circularbuffer_bytes_readable(CircularBuffer buffer);

void circularbuffer_read_buffers(CircularBuffer buffer,
                                 char ** buffer1, int * size1,
                                 char ** buffer2, int * size2,
                                 int bytes_to_read);

void circularbuffer_write_buffers(CircularBuffer buffer,
                                  char ** buffer1, int * size1,
                                  char ** buffer2, int * size2,
                                  int bytes_to_write);

int circularbuffer_insert(CircularBuffer buffer, char * bytes, int length);

int circularbuffer_read(CircularBuffer buffer, char * target, int length);

#endif
