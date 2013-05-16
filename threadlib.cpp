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

ThreadBarrier threadbarrier_make(int nthreads) {
  ThreadBarrier barrier = (ThreadBarrier)malloc(sizeof(struct ThreadBarrier_));
  pthread_mutex_init(&barrier->mutex, NULL);
  pthread_cond_init(&barrier->cond, NULL);
  barrier->nthreads = nthreads;
  barrier->threads_waiting = 0;
  barrier->seq_no = 0;
  return barrier;
}

void threadbarrier_free(ThreadBarrier barrier) {
  pthread_cond_destroy(&barrier->cond);
  pthread_mutex_destroy(&barrier->mutex);
  free(barrier);
}

void threadbarrier_wait(ThreadBarrier barrier) {
  pthread_mutex_lock(&barrier->mutex);
  barrier->threads_waiting += 1;

  if(barrier->threads_waiting == barrier->nthreads) {
    barrier->threads_waiting = 0;
    barrier->seq_no++;
    pthread_cond_broadcast(&barrier->cond);
  } else {
    int last_seq_no = barrier->seq_no;
    while(barrier->seq_no == last_seq_no) {
      pthread_cond_wait(&barrier->cond, &barrier->mutex);
    }
  }

  pthread_mutex_unlock(&barrier->mutex);
}
