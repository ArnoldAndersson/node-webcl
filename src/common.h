// Copyright (c) 2011-2012, Motorola Mobility, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of the Motorola Mobility, Inc. nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef WEBCL_COMMON_H_
#define WEBCL_COMMON_H_

// Node includes
#include <node.h>
#include "nan.h"
#include <string>
#include <list>
#include <set>
#include <map>
#include <memory>
#ifdef LOGGING
#include <iostream>
#endif

using namespace std;

// OpenCL includes
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS

#if defined (__APPLE__) || defined(MACOSX)
  #ifdef __ECLIPSE__
    #include <gltypes.h>
    #include <gl3.h>
    #include <cl_platform.h>
    #include <cl.h>
    #include <cl_gl.h>
    #include <cl_gl_ext.h>
    #include <cl_ext.h>
  #else
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
    #include <OpenGL/OpenGL.h>
    #include <OpenCL/opencl.h>
    #define CL_GL_CONTEXT_KHR 0x2008
    #define CL_EGL_DISPLAY_KHR 0x2009
    #define CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR CL_INVALID_GL_CONTEXT_APPLE
  #endif
  #define HAS_clGetContextInfo
#elif defined(_WIN32)
    #include <GL/gl.h>
    #include <CL/opencl.h>
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
    char *strcasestr(const char *s, char *find);
#else
    #include <GL/gl.h>
    #include <GL/glx.h>
    #include <CL/opencl.h>
#endif

#ifndef CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR
  #define CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR 0x2006 
#endif
#ifndef CL_DEVICES_FOR_GL_CONTEXT_KHR
  #define CL_DEVICES_FOR_GL_CONTEXT_KHR 0x2007 
#endif

// TODO value not defined in spec
#define WEBCL_EXTENSION_NOT_ENABLED 0x8000

namespace {
#define JS_STR(...) v8::String::New(__VA_ARGS__)
#define JS_INT(val) v8::Integer::New(val)
#define JS_NUM(val) v8::Number::New(val)
#define JS_BOOL(val) v8::Boolean::New(val)
#define JS_RETHROW(tc) v8::Local<v8::Value>::New(tc.Exception());

#define REQ_ARGS(N)                                                     \
  if (args.Length() < (N))                                              \
    NanThrowTypeError("Expected " #N " arguments");

#define REQ_STR_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsString())                     \
    NanThrowTypeError("Argument " #I " must be a string");              \
  String::Utf8Value VAR(args[I]->ToString());

#define REQ_EXT_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsExternal())                   \
    NanThrowTypeError("Argument " #I " invalid");                       \
  Local<External> VAR = Local<External>::Cast(args[I]);

#define REQ_FUN_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsFunction())                   \
    NanThrowTypeError("Argument " #I " must be a function");            \
  Local<Function> VAR = Local<Function>::Cast(args[I]);

#define REQ_ERROR_THROW_NONE(error) if (ret == CL_##error) ThrowException(NanObjectWrapHandle(WebCLException::New(#error, ErrorDesc(CL_##error), CL_##error))); return;

#define REQ_ERROR_THROW(error) if (ret == CL_##error) return ThrowException(NanObjectWrapHandle(WebCLException::New(#error, ErrorDesc(CL_##error), CL_##error)));

#define DESTROY_WEBCL_OBJECT(obj)	\
  obj->Destructor();			

#define DISABLE_COPY(ClassName) \
  ClassName( const ClassName& other ); /* non construction-copyable */ \
  ClassName& operator=( const ClassName& ); /* non copyable */

} // namespace

namespace webcl {

const char* ErrorDesc(cl_int err);

// generic baton for async callbacks
struct Baton {
    NanCallback *callback;
    int error;
    char *error_msg;
    uint8_t *priv_info;

    // Custom user data
    v8::Persistent<v8::Value> data;

    // parent of this callback (WebCLEvent object)
    v8::Persistent<v8::Object> parent;

    Baton() : callback(NULL), error(CL_SUCCESS), error_msg(NULL), priv_info(NULL)  {}
    ~Baton() {
      if(error_msg) delete error_msg;
      if(priv_info) delete priv_info;
    }
};

class WebCLObject;
void registerCLObj(void *clid, WebCLObject* obj);
void unregisterCLObj(WebCLObject* obj);
void onExit();

namespace CLObjType {
enum CLObjType {
  None=0,
  Platform,
  Device,
  Context,
  CommandQueue,
  Kernel,
  Program,
  Sampler,
  Event,
  MemoryObject,
  Exception,
  MAX_WEBCL_TYPES
};
static const char* CLObjName[] = {
  "UNKNOWN",
  "Platform",
  "Device",
  "Context",
  "CommandQueue",
  "Kernel",
  "Program",
  "Sampler",
  "Event",
  "MemoryObject",
  "Exception",
};
}

WebCLObject* findCLObj(void* clid, CLObjType::CLObjType type);

class WebCLObject : public node::ObjectWrap {
public:
  inline CLObjType::CLObjType getType() const { return _type; }
  inline const char* getCLObjName() const { return _type<CLObjType::MAX_WEBCL_TYPES ? CLObjType::CLObjName[_type] : "\0"; }

protected:
  WebCLObject() : _type(CLObjType::None)
  {}

  virtual ~WebCLObject() {
#ifdef LOGGING
    printf("[~WebCLObject] %s %p is being destroyed\n",getCLObjName(),this);
#endif
    // unregisterCLObj(this);
  }

public:
  virtual void Destructor() { 
#ifdef LOGGING
    printf("In WebCLObject::Destructor for %s\n",getCLObjName()); 
#endif
  }

  virtual bool operator==(void *clid) { return false; }

  inline void setParent(WebCLObject* parent) {
    _parent=parent;
  }
  
  inline WebCLObject* getParent() const { return _parent; }
  
protected:
  CLObjType::CLObjType _type;
  WebCLObject* _parent;

private:
  DISABLE_COPY(WebCLObject)
};

template <typename T> class Destroyer
{
public:
  Destroyer(T * what)
  {
    this->ptr = what;
  }

  inline void destroy()
  {
    if(!ptr) return;
  #ifdef LOGGING
    printf("[Destroyer] %s\n",ptr->getCLObjName());
  #endif
    // delete ptr;
    ptr->Destructor();
  }

protected:
  T *ptr;
};

template <typename T> class AutoDestroy
{
public:
  static void Add(T * what)
  {
    Instance()->add(what);
  }

  static void Remove(T * what)
  {
    Instance()->remove(what);
  }

  static T* Find(void * what)
  {
    return Instance()->find(what);
  }

  static int Size()
  {
    return Instance()->size();
  }

  virtual ~AutoDestroy()
  {
    clear();
  }

  void clear()
  {
    typename std::set<T *>::iterator i;
    for (i = autoDestroy.begin(); i != autoDestroy.end(); i++)
    {
      #ifdef LOGGING
      printf("[~AutoDestroy] %s %p, refs %d\n",(*i)->getCLObjName(),*i,references[*i]);
      #endif
      // Destroyer<T> destroyer(*i);
      // destroyer.destroy();
      (*i)->Destructor();
    }
    references.clear();
    autoDestroy.clear();
  }

  static AutoDestroy * Instance()
  {
    static AutoDestroy instance;
    return &instance;
  }
protected:
  std::map<T *, int> references;
  std::set<T *> autoDestroy;

  void add(T * what)
  {
    //if it doesn't exist yet, add it.
    if (autoDestroy.count(what) < 1)
    {
      //insert into auto destroy list
      autoDestroy.insert(what);

      //set reference to 1
      references[what] = 0;
    }

    //increment reference
    references[what]++;

#ifdef LOGGING
    printf("Adding %p refCount %d\n",what,references[what]);
#endif
  }

  void remove(T * what)
  {
 #ifdef LOGGING
    printf("Removing %p refCount %d\n",what,references[what]);
 #endif

    //if it doesn't exist, return
    if (autoDestroy.count(what) < 1)
      return;

    //decrement references, and if there aren't any, remove from list.
    if (--references[what] <= 0)
    {
      autoDestroy.erase(what);
      references.erase(what);
    }
  }

  T* find(void * what)
  {
    typename std::set<T *>::iterator i;
    for (i = autoDestroy.begin(); i != autoDestroy.end(); i++)
    {
      WebCLObject *obj = *i;
      if(*obj == what)
        return obj;
    }
    return NULL;
  }

  int size() const
  {
    return references.size();
  }
};

/* Helper functions to make things dead simple */
template<typename T> void autoDestroy(T * what)
{
  AutoDestroy<T>::Add(what);
}

template<typename T> void cancelAutoDestroy(T * what)
{
  AutoDestroy<T>::Remove(what);
}

template<typename T> T* findAutoDestroy(void * what)
{
  return AutoDestroy<T>::Find(what);
}

template<typename T> int sizeAutoDestroy()
{
  return AutoDestroy<T>::Size();
}

} // namespace webcl

#include "exceptions.h"

#endif
