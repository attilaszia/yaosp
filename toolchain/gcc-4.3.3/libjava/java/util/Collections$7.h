
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_util_Collections$7__
#define __java_util_Collections$7__

#pragma interface

#include <java/lang/Object.h>

class java::util::Collections$7 : public ::java::lang::Object
{

public: // actually package-private
  Collections$7(::java::util::Collections$6 *, ::java::util::Map$Entry *);
public:
  jboolean equals(::java::lang::Object *);
  ::java::lang::Object * getKey();
  ::java::lang::Object * getValue();
  jint hashCode();
  ::java::lang::Object * setValue(::java::lang::Object *);
  ::java::lang::String * toString();
public: // actually package-private
  ::java::util::Collections$6 * __attribute__((aligned(__alignof__( ::java::lang::Object)))) this$3;
private:
  ::java::util::Map$Entry * val$e;
public:
  static ::java::lang::Class class$;
};

#endif // __java_util_Collections$7__
