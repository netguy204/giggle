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

void TypeInfo::register_method(MethodInfo* method) {
  name_to_method.insert(std::make_pair(method->name(), method));
}

const PropertyInfo* TypeInfo::property(const char* name) const {
  NameToProperty::const_iterator iter = name_to_property.find(name);
  if(iter == name_to_property.end()) {
    if(m_parent) return m_parent->property(name);
    return NULL;
  }
  return iter->second;
}

const MethodInfo* TypeInfo::method(const char* name) const {
  NameToMethod::const_iterator iter = name_to_method.find(name);
  if(iter == name_to_method.end()) {
    if(m_parent) return m_parent->method(name);
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

const char* TypeInfo::metatable() const {
  if(mt_flag) return name();
  if(m_parent) return m_parent->metatable();
  return NULL;
}

void TypeInfo::set_metatable_flag(bool flag) {
  mt_flag = flag;
}

PropertyInfo::PropertyInfo(TypeInfo* type, const char* name, size_t size)
  : m_type(type), m_name(name), m_size(size) {
  m_type->register_property(this);
}

const char* PropertyInfo::name() const {
  return m_name;
}

size_t PropertyInfo::size() const {
  return m_size;
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

void Object::destroy() {
  delete this;
}

MethodInfo::MethodInfo(TypeInfo* type, const char* name)
  : m_type(type), m_name(name) {
  m_type->register_method(this);
}

const char* MethodInfo::name() const {
  return m_name;
}

Object* LCcheck_object(lua_State *L, int pos) {
  if(lua_isnil(L, pos)) {
    return NULL;
  }

  void** ud = (void**)lua_touserdata(L, pos);
  if(ud == NULL) return NULL;
  return (Object*)*ud;
}

static int Lobject_mutate(lua_State* L) {
  const char* name = luaL_checkstring(L, lua_upvalueindex(1));
  Object* obj = LCcheck_object(L, 1);

  // try invoking a method first
  const MethodInfo* method = obj->typeinfo()->method(name);
  if(method) {
    return method->LCinvoke(L, 1);
  }


  const PropertyInfo* prop = obj->typeinfo()->property(name);
  if(prop == NULL) {
    luaL_error(L, "`%s' does not have property `%s'",
               obj->typeinfo()->name(), name);
  }

  if(lua_gettop(L) == 1) {
    prop->LCpush_value(obj, L);
    return 1;
  } else {
    prop->LCset_value(obj, L, 2);
    return 0;
  }
}

static int Lobject_index(lua_State* L) {
  const char* name = luaL_checkstring(L, 2);
  lua_pushcclosure(L, Lobject_mutate, 1);
  return 1;
}

static int Lobject_tostring(lua_State *L) {
  Object* obj = LCcheck_object(L, 1);
  lua_pushstring(L, obj->typeinfo()->name());
  return 1;
}

static int Lobject_eq(lua_State* L) {
  void** objA = (void**)lua_touserdata(L, 1);
  void** objB = (void**)lua_touserdata(L, 2);
  if(objA != objB && (objA == NULL || objB == NULL)) {
    lua_pushboolean(L, 0);
  } else {
    lua_pushboolean(L, *objA == *objB);
  }
  return 1;
}

static int Lobject_key(lua_State* L) {
  void** obj = (void**)lua_touserdata(L, 1);
  if(obj == NULL) {
    lua_pushnil(L);
  } else {
    lua_pushlightuserdata(L, *obj);
  }
  return 1;
}

static int Lobject_gc(lua_State* L) {
  Object* obj = LCcheck_object(L, 1);
  obj->release();
  return 0;
}

int Lobject_special_gc(lua_State* L) {
  // used by GO and Component, which do not participate in reference
  // counting
  return 0;
}

void LClink_metatable(lua_State *L, const luaL_Reg* table, TypeInfo& type) {
  static const luaL_Reg object_m[] = {
    {"__index", Lobject_index},
    {NULL, NULL}};

  static const luaL_Reg defaults_m[] = {
    {"__tostring", Lobject_tostring},
    {"__eq", Lobject_eq},
    {"__gc", Lobject_gc},
    {"key", Lobject_key},
    {NULL, NULL}};

  luaL_newmetatable(L, type.name());
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);
  lua_settable(L, -3);
  luaL_setfuncs(L, defaults_m, 0);
  if(table) luaL_setfuncs(L, table, 0);

  lua_newtable(L);
  luaL_setfuncs(L, object_m, 0);
  lua_setmetatable(L, -2);

  type.set_metatable_flag(true);
}

void LCinit_object_metatable(lua_State* L) {
  LClink_metatable(L, NULL, Object::Type);
}

void LCpush_lut(lua_State *L, const char* metatable, Object* ut) {
  if(!ut) {
    lua_pushnil(L);
  } else {
    void** p = (void**)lua_newuserdata(L, sizeof(void*));
    luaL_setmetatable(L, metatable);
    *p = ut->retain();
  }
}
