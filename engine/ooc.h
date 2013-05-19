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
#ifndef OOC_H
#define OOC_H

#include <lua.hpp>
#include <stdio.h>
#include <string.h>
#include <map>
#include <typeinfo>

class Object;

typedef Object*(*CtorFn)(void*);

struct cmp_str {
  bool operator()(char const *a, char const *b) const;
};

class PropertyInfo;
class MethodInfo;

typedef std::map<const char*, PropertyInfo*, cmp_str> NameToProperty;
typedef std::map<const char*, MethodInfo*, cmp_str> NameToMethod;

class TypeInfo {
public:
  TypeInfo(const char* name, CtorFn ctor, TypeInfo* parent);
  void register_property(PropertyInfo* property);
  void register_method(MethodInfo* method);

  const PropertyInfo* property(const char* name) const;
  const MethodInfo* method(const char* name) const;

  const char* name() const;
  const TypeInfo* parent() const;
  Object* makeInstance(void*) const;
  bool isInstanceOf(const TypeInfo* other) const;

private:
  NameToProperty name_to_property;
  NameToMethod name_to_method;

  const char* m_name;
  CtorFn m_ctor;
  TypeInfo* m_parent;
};

class PropertyType;

class PropertyInfo {
 public:
  PropertyInfo(TypeInfo* type, const char* name, size_t size);

  const char* name() const;
  size_t size() const;

  virtual void set_value(Object* obj, void* value) const = 0;
  virtual void get_value(Object* obj, void* value) const = 0;
  virtual void LCset_value(Object* obj, lua_State* L,  int pos) const = 0;
  virtual void LCpush_value(Object* obj, lua_State* L) const = 0;

  TypeInfo* m_type;
  size_t m_size;
  const char* m_name;
};

template <typename T>
class PropertyTypeImpl : public PropertyInfo {
public:
  PropertyTypeImpl(TypeInfo* type, const char* name, size_t offset)
    : PropertyInfo(type, name, sizeof(T)), m_offset(offset) {
  }

  virtual void set_value(Object* obj, void* value) const {
    memcpy((char*)obj + m_offset, value, sizeof(T));
  }

  virtual void get_value(Object* obj, void* value) const {
    memcpy(value, (char*)obj + m_offset, sizeof(T));
  }

  virtual void LCset_value(Object* obj, lua_State* L, int pos) const {
    T value;
    get_value(obj, &value);
    LCcheck(L, &value, pos);
    set_value(obj, &value);
  }

  virtual void LCpush_value(Object* obj, lua_State* L) const {
    T value;
    get_value(obj, &value);
    LCpush(L, value);
  }

  size_t m_offset;
};

template<typename T>
inline void LCpush(lua_State* L, T value) {
  luaL_error(L, "don't know how to push `%s'", typeid(value).name());
}

template<typename T>
inline void LCcheck(lua_State* L, T* target, int pos) {
  luaL_error(L, "don't know how to check `%s'", typeid(target).name());
}

template <typename Sig>
class PropertyMethodImpl;

template <typename CLASS, typename T>
class PropertyMethodImpl<void (CLASS::*)(T)> : public PropertyInfo {
public:
  typedef T (CLASS::*Getter)();
  typedef void (CLASS::*Setter)(T);

  PropertyMethodImpl(TypeInfo* type, const char* name,
                     Getter getter, Setter setter)
    : PropertyInfo(type, name, sizeof(T)), m_setter(setter), m_getter(getter) {
  }

  virtual void set_value(Object* _obj, void* _value) const {
    CLASS* obj = (CLASS*)_obj;
    T* value = (T*)_value;
    (obj->*m_setter)(*value);
  }

  virtual void get_value(Object* _obj, void* _value) const {
    CLASS* obj = (CLASS*)_obj;
    T* value = (T*)_value;
    *value = (obj->*m_getter)();
  }

  virtual void LCset_value(Object* obj, lua_State* L, int pos) const {
    T value;
    get_value(obj, &value);
    LCcheck(L, &value, pos);
    set_value(obj, &value);
  }

  virtual void LCpush_value(Object* obj, lua_State* L) const {
    T value;
    get_value(obj, &value);
    LCpush(L, value);
  }

  Getter m_getter;
  Setter m_setter;
};

typedef std::map<const char*, TypeInfo*, cmp_str> NameToType;

class TypeRegistry {
 public:
  static TypeRegistry& instance();

  void register_type(TypeInfo* type);
  TypeInfo* find_type(const char* name);

  NameToType name_to_type;
};

#define OBJECT_PROTO(name)                            \
  virtual const TypeInfo* typeinfo();                 \
  static TypeInfo Type;                               \
  static Object* CreateInstance(void*)


#define OBJECT_BIMPL(name, ptype)                               \
  TypeInfo name::Type(#name, name::CreateInstance, ptype);      \
  const TypeInfo* name::typeinfo() {                            \
    return &(name::Type);                                       \
  }

#define OBJECT_IMPL(name, pname)                        \
  Object* name::CreateInstance(void* init) {            \
    return new name(init);                              \
  }                                                     \
  OBJECT_BIMPL(name, &pname::Type)

#define STRINGIFY(name) #name

#define OP(name, member_name) name ## _ ## member_name ## _ ## Property

#define OBJECT_PROPERTY(name, member_name)                              \
  static PropertyTypeImpl<decltype(((name*)0)->member_name)>            \
    OP(name, member_name)(                                              \
      &name::Type,                                                      \
      #member_name,                                                     \
      offsetof(name, member_name));

#define OBJECT_ACCESSOR(name, member_name, getter, setter)           \
  static PropertyMethodImpl<decltype(&name::setter)>                 \
    OP(name, member_name)(                                           \
      &name::Type,                                                   \
      #member_name,                                                  \
      &name::getter,                                                 \
      &name::setter)

// Methods are added to the metatable for an object. The metatable
// must be fully defined before it is injected into an
// interpreter. Most init should be happening at static initialization
// time so it shouldn't be an issue
class MethodInfo {
 public:
  MethodInfo(TypeInfo* type, const char* name);

  const char* name() const;
  virtual int LCinvoke(lua_State* L, int pos) const = 0;

  TypeInfo* m_type;
  const char* m_name;
};

#define LOP(name, method_name) L ## name ## method_name
#define LOPC(name, method_name) LCLASS ## name ## method_name

#define OBJECT_METHOD_BASE(CLASS, METHOD)                               \
  class LOPC(CLASS, METHOD) : public MethodInfo {                       \
  public:                                                               \
                                                                        \
  LOPC(CLASS, METHOD)()                                                 \
    : MethodInfo(&CLASS::Type, STRINGIFY(METHOD)) {                     \
    }                                                                   \
                                                                        \
    virtual int LCinvoke(lua_State* L, int pos) const;                  \
  };                                                                    \
  static LOPC(CLASS, METHOD) LOP(CLASS, METHOD);                        \
                                                                        \
  int LOPC(CLASS,METHOD)::LCinvoke(lua_State* L, int pos) const

#define OBJECT_METHOD0(CLASS, METHOD, RET)                              \
  OBJECT_METHOD_BASE(CLASS, METHOD) {                                   \
    CLASS* obj;                                                         \
    RET r;                                                              \
    LCcheck(L, &obj, pos);                                              \
    r = obj->METHOD();                                                  \
    LCpush(L, r);                                                       \
    return 1;                                                           \
  }

#define OBJECT_METHOD1(CLASS, METHOD, RET, TYPE1)                       \
  OBJECT_METHOD_BASE(CLASS, METHOD) {                                   \
    CLASS* obj;                                                         \
    RET r;                                                              \
    TYPE1 t1;                                                           \
    LCcheck(L, &obj, pos);                                              \
    LCcheck(L, &t1, pos + 1);                                           \
    r = obj->METHOD(t1);                                                \
    LCpush(L, r);                                                       \
    return 1;                                                           \
  }

#define OBJECT_VMETHOD1(CLASS, METHOD, TYPE1)                           \
  OBJECT_METHOD_BASE(CLASS, METHOD) {                                   \
    CLASS* obj;                                                         \
    TYPE1 t1;                                                           \
    LCcheck(L, &obj, pos);                                              \
    LCcheck(L, &t1, pos + 1);                                           \
    obj->METHOD(t1);                                                    \
    return 0;                                                           \
  }

#define OBJECT_METHOD2(CLASS, METHOD, RET, TYPE1, TYPE2)                \
  OBJECT_METHOD_BASE(CLASS, METHOD) {                                   \
    CLASS* obj;                                                         \
    RET r;                                                              \
    TYPE1 t1;                                                           \
    TYPE2 t2;                                                           \
    LCcheck(L, &obj, pos);                                              \
    LCcheck(L, &t1, pos + 1);                                           \
    LCcheck(L, &t2, pos + 2);                                           \
    r = obj->METHOD(t1, t2);                                            \
    LCpush(L, r);                                                       \
    return 1;                                                           \
  }

#define OBJECT_METHOD3(CLASS, METHOD, RET, TYPE1, TYPE2, TYPE3)         \
  OBJECT_METHOD_BASE(CLASS, METHOD) {                                   \
    CLASS* obj;                                                         \
    RET r;                                                              \
    TYPE1 t1;                                                           \
    TYPE2 t2;                                                           \
    TYPE3 t3;                                                           \
    LCcheck(L, &obj, pos);                                              \
    LCcheck(L, &t1, pos + 1);                                           \
    LCcheck(L, &t2, pos + 2);                                           \
    LCcheck(L, &t3, pos + 3);                                           \
    r = obj->METHOD(t1, t2, t3);                                        \
    LCpush(L, r);                                                       \
    return 1;                                                           \
  }


class Object {
public:
  OBJECT_PROTO(Object);
};


// specialize the lua getters and setters for the primitives here
// since we'll always need those.
template<>
inline void LCpush<int>(lua_State* L, int val) {
  lua_pushinteger(L, val);
}

template<>
inline void LCcheck(lua_State* L, int* target, int pos) {
  *target = luaL_checkinteger(L, pos);
}

template<>
inline void LCpush<long>(lua_State* L, long val) {
  lua_pushinteger(L, val);
}

template<>
inline void LCcheck<long>(lua_State* L, long* target, int pos) {
  *target = luaL_checkinteger(L, pos);
}

template<>
inline void LCpush<float>(lua_State* L, float val) {
  lua_pushnumber(L, val);
}

template<>
inline void LCcheck<float>(lua_State* L, float* target, int pos) {
  *target = luaL_checknumber(L, pos);
}

template<>
inline void LCpush<const char*>(lua_State* L, const char* str) {
  lua_pushstring(L, str);
}

template<>
inline void LCcheck<const char*>(lua_State* L, const char** str, int pos) {
  *str = luaL_checkstring(L, pos);
}

#endif
