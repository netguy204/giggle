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

ThreadBarrier::ThreadBarrier(int nthreads)
  : nthreads(nthreads), threads_waiting(0), seq_no(0) {
  mutex = mutex_create();
  cond = condition_create();
}

ThreadBarrier::~ThreadBarrier() {
  condition_destroy(cond);
  mutex_destroy(mutex);
}

void ThreadBarrier::wait() {
  mutex_lock(mutex);
  threads_waiting += 1;

  if(threads_waiting == nthreads) {
    threads_waiting = 0;
    seq_no++;
    condition_broadcast(cond);
  } else {
    int last_seq_no = seq_no;
    while(seq_no == last_seq_no) {
      condition_wait(cond, mutex);
    }
  }

  mutex_unlock(mutex);
}



ThreadEvent::ThreadEvent()
  : triggered(false), seq_no(0) {
  mutex = mutex_create();
  cond = condition_create();
}

void ThreadEvent::wait() {
  if(triggered) return;

  mutex_lock(mutex);
  int last_seq_no = seq_no;
  while(!triggered && last_seq_no == seq_no) {
    condition_wait(cond, mutex);
  }
  mutex_unlock(mutex);
}

void ThreadEvent::notify() {
  mutex_lock(mutex);
  triggered = true;
  seq_no++;
  condition_broadcast(cond);
  mutex_unlock(mutex);
}

bool ThreadEvent::has_fired() {
  bool result;
  mutex_lock(mutex);
  result = triggered;
  mutex_unlock(mutex);
  return result;
}

void ThreadEvent::reset() {
  mutex_lock(mutex);
  triggered = false;
  mutex_unlock(mutex);
}
