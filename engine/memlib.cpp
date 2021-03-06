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
#include "memlib.h"
#include "utils.h"
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>


/**
 * FixedAllocator's are used to quickly allocate and free objects of
 * fixed size. They operate in constant time but cannot allocate more
 * objects than they were initially designed to hold. This makes them
 * appropriate for holding things like resource handles (since the
 * number of resources in the system is finite), timelines, and other
 * finite arity and long duration objects.
 */
FixedAllocator::FixedAllocator(size_t obj_size, unsigned int n,
                               const char* name) {
  int ii;

  /* next 8 byte aligned size */
  obj_size = NEXT_ALIGNED_SIZE(obj_size);
  void* mem = malloc(obj_size * n);

#ifdef DEBUG_MEMORY
  this->name = name;
  inflight = 0;
  max_inflight = 0;
#endif

  mutex = mutex_create();
  allocation_size = obj_size;

  first_free = NULL;
  for(ii = 0; ii < n; ++ii) {
    *(void**)mem = first_free;
    first_free = mem;
    mem = (char*)mem + obj_size;
  }
}

void* FixedAllocator::alloc() {
  SAFETY(if(!first_free)
           return fail_exit("fixed_allocator %s failed", name));

  mutex_lock(mutex);
  void * mem = first_free;
  first_free = *(void**)first_free;

#ifdef DEBUG_MEMORY
  inflight += 1;
  if(inflight > max_inflight) {
    max_inflight = inflight;
  }
#endif
  mutex_unlock(mutex);

  return mem;
}

void FixedAllocator::free(void *obj) {
  mutex_lock(mutex);
  *(void**)obj = first_free;
  first_free = obj;

#ifdef DEBUG_MEMORY
  inflight -= 1;
#endif
  mutex_unlock(mutex);
}

StackAllocator stack_allocator_make(size_t stack_size, const char* name) {
  size_t size = sizeof(struct StackAllocator_) * 2 + stack_size;
  size = NEXT_ALIGNED_SIZE(size);

  StackAllocator allocator = (StackAllocator)malloc(size);
#ifdef DEBUG_MEMORY
  allocator->name = name;
#endif

  allocator->stack_bottom = &allocator[1];
  allocator->stack_top = allocator->stack_bottom;
  allocator->stack_max = (char*)allocator->stack_top + stack_size;
  return allocator;
}

void* stack_allocator_alloc(StackAllocator allocator, size_t size) {
  size = NEXT_ALIGNED_SIZE(size);

  SAFETY(if((char*)allocator->stack_top + size > (char*)allocator->stack_max) return fail_exit("stack_allocator %s failed", allocator->name));
  void* mem = allocator->stack_top;
  allocator->stack_top = (char*)allocator->stack_top + size;

  return mem;
}

void stack_allocator_freeall(StackAllocator allocator) {
#ifdef DEBUG_MEMORY
  allocator->max_alloced = (char*)allocator->stack_top - (char*)allocator->stack_bottom;
#endif
  allocator->stack_top = allocator->stack_bottom;
}

void stack_allocator_release(StackAllocator allocator) {
  free(allocator);
}

CircularBuffer circularbuffer_make(size_t bytes) {
  CircularBuffer buffer = (CircularBuffer)malloc(sizeof(struct CircularBuffer_));
  buffer->read_index = 0;
  buffer->write_index = 0;
  buffer->size = bytes;
  buffer->filled = 0;
  buffer->data = (char*)malloc(bytes);
  return buffer;
}

void circularbuffer_free(CircularBuffer buffer) {
  free(buffer->data);
  free(buffer);
}

int circularbuffer_distance(CircularBuffer buffer, int i1, int i2) {
  if(i2 > i1) {
    return i2 - i1;
  } else {
    return i2 + buffer->size - i1;
  }
}

int circularbuffer_bytes_writable(CircularBuffer buffer) {
  if(!buffer->filled && (buffer->write_index == buffer->read_index)) {
    return buffer->size;
  } else if(buffer->filled) {
    return 0;
  } else {
    return circularbuffer_distance(buffer, buffer->write_index,
                                   buffer->read_index);
  }
}

int circularbuffer_bytes_readable(CircularBuffer buffer) {
  if(buffer->filled) {
    return buffer->size;
  } else if(buffer->read_index == buffer->write_index) {
    return 0;
  } else {
    return circularbuffer_distance(buffer, buffer->read_index,
                                   buffer->write_index);
  }
}

int circularbuffer_add(CircularBuffer buffer, int a, int b) {
  return (a + b) % buffer->size;
}

void circularbuffer_read_buffers(CircularBuffer buffer,
                                 char ** buffer1, int * size1,
                                 char ** buffer2, int * size2,
                                 int bytes_to_read) {
  // easy case, 1 buffer
  if(buffer->read_index < buffer->write_index) {
    *buffer1 = &buffer->data[buffer->read_index];
    *size1 = buffer->write_index - buffer->read_index;
    *buffer2 = NULL;
    *size2 = 0;
  } else if(!buffer->filled && buffer->read_index == buffer->write_index) {
    *buffer1 = NULL;
    *size1 = 0;
    *buffer2 = NULL;
    *size2 = 0;
  } else {
    // 2 buffers
    *buffer1 = &buffer->data[buffer->read_index];
    *size1 = buffer->size - buffer->read_index;
    *buffer2 = buffer->data;
    *size2 = buffer->write_index;
  }

  bytes_to_read = MIN(bytes_to_read, *size1 + *size2);
  buffer->read_index = circularbuffer_add(buffer, buffer->read_index,
                                          bytes_to_read);

  if(bytes_to_read > 0) buffer->filled = 0;
}

void circularbuffer_write_buffers(CircularBuffer buffer,
                                  char ** buffer1, int * size1,
                                  char ** buffer2, int * size2,
                                  int bytes_to_write) {
  // 1 buffer
  if(buffer->write_index < buffer->read_index) {
    *buffer1 = &buffer->data[buffer->write_index];
    *size1 = buffer->read_index - buffer->write_index;
    *buffer2 = NULL;
    *size2 = 0;
  } else if (buffer->filled) {
    *buffer1 = NULL;
    *size1 = 0;
    *buffer2 = NULL;
    *size2 = 0;
  } else {
    // the corner case and the 2 buffer case are the same for writing
    *buffer1 = &buffer->data[buffer->write_index];
    *size1 = buffer->size - buffer->write_index;
    *buffer2 = buffer->data;
    *size2 = buffer->read_index;
  }

  bytes_to_write = MIN(bytes_to_write, *size1 + *size2);
  buffer->write_index = circularbuffer_add(buffer, buffer->write_index,
                                           bytes_to_write);

  if(bytes_to_write > 0 && buffer->read_index == buffer->write_index) {
    buffer->filled = 1;
  }
}

int circularbuffer_insert(CircularBuffer buffer, char * bytes, int length) {
  assert(bytes);

  int s1, s2;
  char *b1, *b2;
  circularbuffer_write_buffers(buffer, &b1, &s1, &b2, &s2, length);

  int to_write = MIN(length, s1);

  assert(b1);
  memcpy(b1, bytes, to_write);
  length -= to_write;

  if(length > 0) {
    to_write = MIN(length, s2);
    memcpy(b2, &bytes[s1], to_write);
    length -= to_write;
  }

  return length == 0;
}

int circularbuffer_read(CircularBuffer buffer, char * target, int length) {
  assert(target);

  int s1, s2;
  char *b1, *b2;
  circularbuffer_read_buffers(buffer, &b1, &s1, &b2, &s2, length);

  int to_read = MIN(length, s1);

  assert(b1);
  memcpy(target, b1, to_read);
  length -= to_read;

  if(length > 0) {
    to_read = MIN(length, s2);
    memcpy(&target[s1], b2, to_read);
    length -= to_read;
  }

  return length == 0;
}
