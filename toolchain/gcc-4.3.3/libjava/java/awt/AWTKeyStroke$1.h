
// DO NOT EDIT THIS FILE - it is machine generated -*- c++ -*-

#ifndef __java_awt_AWTKeyStroke$1__
#define __java_awt_AWTKeyStroke$1__

#pragma interface

#include <java/util/LinkedHashMap.h>
extern "Java"
{
  namespace java
  {
    namespace awt
    {
        class AWTKeyStroke$1;
    }
  }
}

class java::awt::AWTKeyStroke$1 : public ::java::util::LinkedHashMap
{

public: // actually package-private
  AWTKeyStroke$1(jint, jfloat, jboolean);
public: // actually protected
  jboolean removeEldestEntry(::java::util::Map$Entry *);
private:
  static const jint MAX_CACHE_SIZE = 2048;
public:
  static ::java::lang::Class class$;
};

#endif // __java_awt_AWTKeyStroke$1__