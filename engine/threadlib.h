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

#ifdef BUILD_SDL
#include <SDL2/SDL_thread.h>

#define Thread SDL_Thread*
#define Mutex SDL_mutex*
#define Condition SDL_cond*

typedef int(*THREAD_FN)(void*);
#define thread_create(fn, arg) SDL_CreateThread((THREAD_FN)fn, NULL, (void*)arg)

#define mutex_create() SDL_CreateMutex()
#define mutex_destroy(m) SDL_DestroyMutex(m)
#define mutex_lock(m) SDL_LockMutex(m)
#define mutex_unlock(m) SDL_UnlockMutex(m)

#define condition_create() SDL_CreateCond()
#define condition_destroy(c) SDL_DestroyCond(c)
#define condition_signal(c) SDL_CondSignal(c)
#define condition_broadcast(c) SDL_CondBroadcast(c)
#define condition_wait(c, m) SDL_CondWait(c, m)

#else
#include <pthread.h>
#error "pthread bindings incomplete"
#endif

#include "listlib.h"

template<typename E, int OFFSET>
class Queue {
 public:
  DLL<E,OFFSET> list;
  Mutex mutex;
  Condition cond;

  Queue() {
    mutex = mutex_create();
    cond = condition_create();
  }

  ~Queue() {
    condition_destroy(cond);
    mutex_destroy(mutex);
  }

  void enqueue(E* element) {
    mutex_lock(mutex);
    this->list.add_head(element);
    mutex_unlock(mutex);

    condition_signal(cond);
  }

  E* dequeue() {
    mutex_lock(mutex);
    while(1) {
      if(this->list.tail) {
        E* result = this->list.remove_tail();
        mutex_unlock(mutex);
        return result;
      } else {
        /* need to wait for something to be put in the queue */
        condition_wait(cond, mutex);
      }
    }
  }

  E* dequeue_noblock() {
    mutex_lock(mutex);
    E* result = NULL;
    if(this->list.tail) {
      result = this->list.remove_tail();
    }
    mutex_unlock(mutex);
    return result;
  }
};

class ThreadBarrier {
private:
  Mutex mutex;
  Condition cond;
  int nthreads;
  int threads_waiting;
  int seq_no; // number of times we've waited at this barrier, overflow ok

public:
  ThreadBarrier(int nthreads);
  ~ThreadBarrier();

  void wait();
};

class ThreadEvent {
private:
  Mutex mutex;
  Condition cond;
  bool triggered;
  int seq_no;

public:
  ThreadEvent();

  void wait();
  void notify();
  bool has_fired();
  void reset();
};

#endif
