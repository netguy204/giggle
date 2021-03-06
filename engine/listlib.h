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
#ifndef LISTLIB_H
#define LISTLIB_H

#include <assert.h>
#include <stddef.h>

void listlib_init();

typedef struct DLLNode_ *DLLNode;

struct DLLNode_ {
  DLLNode next;
  DLLNode prev;
};

class SimpleDLL {
 public:
  SimpleDLL();

  DLLNode head;
  DLLNode tail;
  int nelems;

  void add_head_node(DLLNode node);
  void add_tail_node(DLLNode node);
  void remove_node(DLLNode node);
  DLLNode remove_tail_node();
  int contains_node(DLLNode node);

  int count();
  void zero();
  int is_empty();
  void insert_after(DLLNode target, DLLNode addition);
  void insert_before(DLLNode target, DLLNode addition);

  template<typename Func>
  void foreach_node(Func func) {
    DLLNode node = this->head;
    while(node) {
      DLLNode next = node->next;
      if(func(node)) return;
      node = next;
    }
  }
};

template<typename E, int OFFSET>
class DLL : public SimpleDLL {
 public:

  void add_head(E* element) {
    DLLNode addition = DLL<E,OFFSET>::to_node(element);
    add_head_node(addition);
  }

  void add_tail(E* element) {
    DLLNode addition = DLL<E,OFFSET>::to_node(element);
    add_tail_node(addition);
  }

  void remove(E* element) {
    DLLNode node = DLL<E,OFFSET>::to_node(element);
    remove_node(node);
  }

  E* remove_tail() {
    DLLNode node = remove_tail_node();
    return DLL<E,OFFSET>::to_element(node);
  }

  E* head_element() {
    if(!this->head) return NULL;
    return DLL<E,OFFSET>::to_element(this->head);
  }

  E* tail_element() {
    if(!this->tail) return NULL;
    return DLL<E,OFFSET>::to_element(this->tail);
  }

  int contains(E* element) {
    return contains_node(to_node(element));
  }

  DLLNode to_node(E* element) {
    return (DLLNode)((char*)element + OFFSET);
  }

  E* to_element(DLLNode node) {
    return (E*)((char*)node - OFFSET);
  }

  template<typename Func>
  void foreach(Func func) {
    DLLNode node = this->head;
    while(node) {
      DLLNode next = node->next;
      if(func(DLL<E,OFFSET>::to_element(node))) return;
      node = next;
    }
  }

  template<typename Func>
  void insert_before_when(E* element, Func func) {
    E* h = head_element();

    if(!h || func(h)) {
      add_head(element);
      return;
    }

    int found = 0;
    DLLNode node = this->head;
    while(node) {
      DLLNode next = node->next;
      E* e = DLL<E,OFFSET>::to_element(node);
      if(e != h && func(e)) {
        insert_before(DLL<E,OFFSET>::to_node(e), DLL<E,OFFSET>::to_node(element));
        found = 1;
        break;
      }
      node = next;
    }

    if(!found) {
      add_tail(element);
    }
  }
};

#define DLL_DECLARE(TYPE, LINKNAME) DLL<TYPE, offsetof(TYPE, LINKNAME)>

#endif
