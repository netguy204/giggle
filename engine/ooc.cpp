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
#include "ooc.h"

TypeInfo::TypeInfo(const char* name, CtorFn ctor, TypeInfo* parent)
  : m_name(name), m_ctor(ctor), m_parent(parent) {
  TypeRegistry::instance().register_type(this);
}

void TypeInfo::register_property(PropertyInfo* property) {
  name_to_property.insert(std::make_pair(property->name(), property));
}

const PropertyInfo* TypeInfo::property(const char* name) const {
  NameToProperty::const_iterator iter = name_to_property.find(name);
  if(iter == name_to_property.end()) {
    if(m_parent) return m_parent->property(name);
    return NULL;
  }
  return iter->second;
}

const char* TypeInfo::name() const {
  return m_name;
}

const TypeInfo* TypeInfo::parent() const {
  return m_parent;
}

Object* TypeInfo::makeInstance(void* init) const {
  return m_ctor(init);
}

bool TypeInfo::isInstanceOf(const TypeInfo* other) const {
  // add superclass info
  if(this == other) return true;
  if(!parent()) return false;
  return parent()->isInstanceOf(other);
}

PropertyInfo::PropertyInfo(TypeInfo* type, const char* name)
  : m_type(type), m_name(name) {
  m_type->register_property(this);
}

const char* PropertyInfo::name() const {
  return m_name;
}

bool cmp_str::operator()(char const *a, char const *b) const {
  return strcmp(a, b) < 0;
}

TypeRegistry& TypeRegistry::instance() {
  static TypeRegistry registry;
  return registry;
}

void TypeRegistry::register_type(TypeInfo* type) {
  name_to_type.insert(std::make_pair(type->name(), type));
}

TypeInfo* TypeRegistry::find_type(const char* name) {
  NameToType::iterator iter = name_to_type.find(name);
  if(iter == name_to_type.end()) return NULL;
  return iter->second;
}

OBJECT_BIMPL(Object, NULL);

Object* Object::CreateInstance(void* init) {
  return new Object();
}
