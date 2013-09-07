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

void SimpleDLL::add_head_node(DLLNode addition) {
  if(this->head == NULL) {
    addition->next = NULL;
    addition->prev = NULL;
    this->head = addition;
    this->tail = addition;
    ++this->nelems;
  } else {
    insert_before(this->head, addition);
    this->head = addition;
  }
}

void SimpleDLL::add_tail_node(DLLNode addition) {
  if(this->tail == NULL) {
    addition->next = NULL;
    addition->prev = NULL;
    this->head = addition;
    this->tail = addition;
    ++this->nelems;
  } else {
    insert_after(this->tail, addition);
    this->tail = addition;
  }
}

void SimpleDLL::remove_node(DLLNode node) {
  if(this->head == node) {
    DLLNode next = node->next;
    if(next) {
      next->prev = NULL;
      this->head = node->next;
    } else {
      this->head = NULL;
      this->tail = NULL;
    }

    --this->nelems;
    return;
  }

  if(this->tail == node) {
    this->remove_tail_node();
    return;
  }

  DLLNode after = node->next;
  DLLNode before = node->prev;

  if(after) {
    assert(after->prev == node);
    after->prev = before;
  }

  if(before) {
    assert(before->next == node);
    before->next = after;
  }
  --this->nelems;
}

DLLNode SimpleDLL::remove_tail_node() {
  if (this->tail == NULL) return NULL;

  DLLNode result = this->tail;
  DLLNode before = result->prev;
  if(before) {
    before->next = NULL;
    this->tail = before;
  } else {
    this->head = NULL;
    this->tail = NULL;
  }
  --this->nelems;
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

void SimpleDLL::insert_after(DLLNode target, DLLNode addition) {
  DLLNode after = target->next;
  target->next = addition;
  addition->prev = target;
  addition->next = after;
  if(after) {
    after->prev = addition;
  }
  ++this->nelems;
}

void SimpleDLL::insert_before(DLLNode target, DLLNode addition) {
  DLLNode before = target->prev;
  target->prev = addition;
  addition->next = target;
  addition->prev = before;
  if(before) {
    before->next = addition;
  }
  ++this->nelems;
}
