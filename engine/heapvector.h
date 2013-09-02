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
#ifndef HEAPVECTOR_H
#define HEAPVECTOR_H

#include <stdlib.h>
#include <memory.h>

#include <vector>

template<typename T, typename A = std::allocator<T> >
class BinaryHeap {
private:

  void swap(int posa, int posb) {
    T elm = heap[posa];
    heap[posa] = heap[posb];
    heap[posb] = elm;
  }

  int compare(int posa, int posb) {
    return comparison(heap[posa], heap[posb]);
  }


public:
  typedef int(*BinaryHeapComparison)(const T& a, const T& b);

  std::vector<T, A> heap;
  BinaryHeapComparison comparison;

  inline BinaryHeap(BinaryHeapComparison comparison)
    : comparison(comparison) {
  }

  inline BinaryHeap(BinaryHeapComparison comparison, const A& allocator)
    : heap(allocator), comparison(comparison) {
  }

  void insert(const T& value) {
    // inserts start at the bottom and bubble up
    heap.push_back(value);

    // positions are index + 1
    int test_pos = heap.size();

    while(test_pos != 1) {
      int parent = test_pos / 2;
      if(compare(test_pos-1, parent-1) <= 0) {
        swap(test_pos-1, parent-1);
        test_pos = parent;
      } else {
        // we're done!
        break;
      }
    }
  }

  const T& top() {
    return heap.front();
  }

  void pop() {
    // remove the top by inserting the bottom element at the top and
    // letting it bubble down
    heap.front() = heap.back();
    heap.pop_back();

    size_t sz = size();

    int v = 1;
    int u;
    while(1) {
      u = v;

      int left_child = 2 * u;
      int right_child = left_child + 1;
      if(right_child <= sz) {
        // need to pick the smaller of our two children to swap with
        if(compare(u-1, left_child-1) >= 0) v = left_child;
        if(compare(v-1, right_child-1) >= 0) v = right_child;
      } else if(left_child <= sz) {
        // only have one child to consider
        if(compare(u-1, left_child-1) >= 0) v = left_child;
      }

      if(u != v) {
        swap(u-1, v-1);
      } else {
        break; // done!
      }
    }
  }


  size_t size() const {
    return heap.size();
  }

  bool empty() const {
    return heap.empty();
  }
};

#endif
