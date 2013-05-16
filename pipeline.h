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
#ifndef PIPELINE_H
#define PIPELINE_H

#include "listlib.h"

class Pipeline;

class PipelineElement {
 public:
  PipelineElement();
  virtual ~PipelineElement();

  virtual void update(long delta) = 0;

  void callNext(long delta);

  struct DLLNode_ node;
  Pipeline* pipeline;
};

class Pipeline {
 public:
  Pipeline();

  virtual void update(long delta);
  void add_front(PipelineElement* element);
  void add_back(PipelineElement* element);

  DLL_DECLARE(PipelineElement, node) elements;
};

#endif
