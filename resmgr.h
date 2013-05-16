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
#ifndef RESMGR_H
#define RESMGR_H

#include "ooc.h"
#include <map>

typedef void*(*Loader)(void*);
typedef std::pair<Loader,void*> LoaderAndArg;
typedef std::map<LoaderAndArg, Promise> PromiseCache;

class Resource {

  virtual void ensureLoaded();
};

class ResourceLoader : public Object {
 public:
  ResourceLoader();


};

class ImageLoader : public ResourceLoader {
 public:
  ImageLoader();


};

class ResourceManager : public Object {

};

#endif
