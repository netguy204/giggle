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
#ifndef THREADLIB_H
#define THREADLIB_H

#include <stdlib.h>
#include <pthread.h>
#include "listlib.h"

template<typename E, int OFFSET>
class Queue {
 public:
  DLL<E,OFFSET> list;
  pthread_mutex_t mutex;
  pthread_cond_t cond;

  Queue() {
    pthread_mutex_init(&this->mutex, NULL);
    pthread_cond_init(&this->cond, NULL);
  }

  ~Queue() {
    pthread_cond_destroy(&this->cond);
    pthread_mutex_destroy(&this->mutex);
  }

  void enqueue(E* element) {
    pthread_mutex_lock(&this->mutex);
    this->list.add_head(element);
    pthread_mutex_unlock(&this->mutex);

    pthread_cond_signal(&this->cond);
  }

  E* dequeue() {
    pthread_mutex_lock(&this->mutex);
    while(1) {
      if(this->list.tail) {
        E* result = this->list.remove_tail();
        pthread_mutex_unlock(&this->mutex);
        return result;
      } else {
        /* need to wait for something to be put in the queue */
        pthread_cond_wait(&this->cond, &this->mutex);
      }
    }
  }

  E* dequeue_noblock() {
    pthread_mutex_lock(&this->mutex);
    E* result = NULL;
    if(this->list.tail) {
      result = this->list.remove_tail();
    }
    pthread_mutex_unlock(&this->mutex);
    return result;
  }
};

typedef struct ThreadBarrier_ {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  int nthreads;
  int threads_waiting;
  int seq_no; // number of times we've waited at this barrier, overflow ok
} *ThreadBarrier;

ThreadBarrier threadbarrier_make(int nthreads);
void threadbarrier_free(ThreadBarrier barrier);

void threadbarrier_wait(ThreadBarrier barrier);

#endif
