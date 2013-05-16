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
#include "pipeline.h"

PipelineElement::PipelineElement()
  : pipeline(NULL) {
}

PipelineElement::~PipelineElement() {
  if(pipeline) {
    pipeline->elements.remove(this);
  }
}

void PipelineElement::callNext(long delta) {
  DLLNode next = node.next;
  if(!next) return;
  PipelineElement* element = pipeline->elements.to_element(next);
  element->update(delta);
}

Pipeline::Pipeline() {
}

void Pipeline::update(long delta) {
  // forward update
  DLLNode node = elements.head;
  if(!node) return;
  PipelineElement* element = elements.to_element(node);
  element->update(delta); // each element delegates to next
}

void Pipeline::add_front(PipelineElement* element) {
  element->pipeline = this;
  elements.add_head(element);
}

void Pipeline::add_back(PipelineElement* element) {
  element->pipeline = this;
  elements.add_tail(element);
}
