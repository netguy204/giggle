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
#include "promise.h"
#include "utils.h"

Promise::Promise()
  : value(NULL) {
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);
}

Promise::~Promise() {
  pthread_cond_destroy(&cond);
  pthread_mutex_destroy(&mutex);
}

void* Promise::get() {
  if(value) return value;

  pthread_mutex_lock(&mutex);
  while(!value) {
    pthread_cond_wait(&cond, &mutex);
  }
  pthread_mutex_unlock(&mutex);
  return value;
}

void Promise::deliver(void* _value) {
  if(value) {
    fail_exit("cannot deliver a promise twice");
  }

  pthread_mutex_lock(&mutex);
  value = _value;
  pthread_mutex_unlock(&mutex);
  pthread_cond_broadcast(&cond);
}
