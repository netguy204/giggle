#ifndef METACLASS_H
#define METACLASS_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <new>
#define ABS(x) (x < 0 ? -x : x)
#define NEXT_ALIGNED_SIZE(x) ((x + 8 - 1) & ~(8 - 1))

// The "type" of a pointer to a member function is not specified. In
// the case of gcc and clang it's actually an offset (in bytes) from
// the start of the vtable to the function it refers to.
template<typename T>
unsigned int member_offset(T mfun) {
  void* ptr = *(void**)&mfun;
  return ((uintptr_t)ptr) / sizeof(void*);
}

// means that no additional storage is necessary for the dynamic
// subtype
struct NULL_TYPE {};

/**
 * A metaclass for any single inheritance c++ class that uses the
 * virtual keyword somewhere in its heirarchy. Only virtual methods
 * with be able to be overridden at runtime.
 */
template<typename T, typename U = NULL_TYPE>
class Metaclass {
public:
  // The location of a vtable is not defined by the spec. This
  // particular implementation works in gcc and clang and I haven't
  // tested it anywhere else.
  void **vtable_get(const T* obj) {
    return *(void***)obj;
  }

  // This relies on the assumption that most of the members functions
  // of a given class are "near" each other. This assumption could be
  // violated if the class in question overrides a few methods on a
  // class that whose base is in a shared library. I'm not sure how to
  // make this more robust. It would probably be fine just to always
  // make "big" vtables instead of trying to determine the minimal
  // vtable and assume no-one defines more than 100 methods or so on a
  // class.
  unsigned int vtable_nummembers(const T* obj) {
    void** vtable = vtable_get(obj);

    // assume at least one virtual function (safe per our contract)
    void* first = vtable[0];

    // assume that all other functions are within 1 MB of the first
    // (reasonable but magic)
    for(int ii = 1; ii < 100; ++ii) { // no more than 100 methods (please!)
      ptrdiff_t diff = (ptrdiff_t)vtable[ii] - (ptrdiff_t)first;
      if(ABS((ptrdiff_t)diff) > 0x10000) {
        return ii;
      }
    }
    return 0;
  }

  void** vtable_clone(const T* obj) {
    void** vtable = vtable_get(obj);

    unsigned int count = vtable_nummembers(obj) + 1;
    void** newtable = (void**)malloc(sizeof(void*) * count);

    memcpy(newtable, vtable, sizeof(void*) * count);
    return newtable;
  }

  typedef Metaclass<T,U> TYPE;
  typedef void(*Destructor)(T* obj);

  void** vtable;
  size_t data_size;
  Destructor dtor;

  struct NewObject {
    T obj;
    TYPE* metaclass;
    U user;
  };

public:
  static U* userdata(T* obj) {
    NewObject* nobj = (NewObject*)obj;
    return &nobj->user;
  }

  // invoked when only the destructor should be executed. EG, explicit
  // destructor call
  static void virtual_destructor0(T* obj) {
    U* hidden = userdata(obj);
    Destructor dtor = ((NewObject*)obj)->metaclass->dtor;
    hidden->~U();
    dtor(obj);
  }

  // invoked when the storage backing this object should be released
  static void virtual_destructor1(T* obj) {
    U* hidden = userdata(obj);
    Destructor dtor = ((NewObject*)obj)->metaclass->dtor;
    hidden->~U();
    dtor(obj);
    free(obj);
  }

  Metaclass(const T* archetype) {
    vtable = vtable_clone(archetype);
    data_size = sizeof(NewObject);

    // the location of the in-place destructor and memory freeing
    // destructors is probably implementation specific. Again, this is
    // the right choice for gcc and clang.
    dtor = (Destructor)vtable[0];

    // install virtual destructors
    override_slot(0, virtual_destructor0);
    override_slot(1, virtual_destructor1);
  }

  // it's important that the meta class live as long as all of the
  // objects that it creates because freeing it will make the vtable
  // for those objects invalid.
  ~Metaclass() {
    free(vtable);
  }

  T* apply_vtable(T* obj) {
    // install our new vtable. Again, the location of the vtable is
    // not specified but this choice works in gcc and clang.
    *(void**)obj = (void*)vtable;

    // also stash the metaclass so we can look up important stuff
    ((NewObject*)obj)->metaclass = this;

    return obj;
  }

  template<typename A, typename B>
  void override(A memfn, B* newfn) {
    vtable[member_offset(memfn)] = (void*)newfn;
  }

  template<typename B>
  void override_slot(unsigned int slot, B* newfn) {
    vtable[slot] = (void*)newfn;
  }

  void* alloc() {
    return (void*)malloc(data_size);
  }

  T* spawn() {
    void* mem = alloc();
    T* obj = new (mem) T();
    return apply_vtable(obj);
  }
};

// create a new dynamic subtype instance of T using meta. Remaining
// arguments are passed to the constructor for T
#define SPAWN(T, meta, ...) meta.apply_vtable(new(meta.alloc()) T(__VA_ARGS__))

#endif
