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
#include "listlib.h"
#include "memlib.h"
#include "utils.h"
#include "config.h"

#include <assert.h>

SimpleDLL::SimpleDLL()
  : head(NULL), tail(NULL), nelems(0) {
}

void SimpleDLL::insert_after(DLLNode target, DLLNode addition) {
  addition->prev = target;
  addition->next = target->next;
  if(!target->next) {
    tail = addition;
  } else {
    target->next->prev = addition;
  }
  target->next = addition;
  ++this->nelems;
}

void SimpleDLL::insert_before(DLLNode target, DLLNode addition) {
  addition->prev = target->prev;
  addition->next = target;
  if(!target->prev) {
    head = addition;
  } else {
    target->prev->next = addition;
  }
  target->prev = addition;
  ++this->nelems;
}

void SimpleDLL::add_head_node(DLLNode addition) {
  if(!head) {
    head = addition;
    tail = addition;
    addition->next = NULL;
    addition->prev = NULL;
    ++this->nelems;
  } else {
    insert_before(this->head, addition);
  }
}

void SimpleDLL::add_tail_node(DLLNode addition) {
  if(!tail) {
    add_head_node(addition);
  } else {
    insert_after(tail, addition);
  }
}

void SimpleDLL::remove_node(DLLNode node) {
  if(!node->prev) {
    head = node->next;
  } else {
    node->prev->next = node->next;
  }

  if(!node->next) {
    tail = node->prev;
  } else {
    node->next->prev = node->prev;
  }
  --this->nelems;
}

DLLNode SimpleDLL::remove_tail_node() {
  if(!tail) return NULL;
  DLLNode result = tail;
  remove_node(tail);
  return result;
}

int SimpleDLL::contains_node(DLLNode test) {
  DLLNode node = head;
  while(node) {
    if(node == test) return 1;
    node = node->next;
  }
  return 0;
}

int SimpleDLL::count() {
  return this->nelems;
}

void SimpleDLL::zero() {
  this->head = NULL;
  this->tail = NULL;
  this->nelems = 0;
}

int SimpleDLL::is_empty() {
  return this->head == NULL;
}
