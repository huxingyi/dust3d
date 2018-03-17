// Copyright (c) 2006, Tobias Sargeant
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the
// distribution.  The names of its contributors may be used to endorse
// or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <iostream>
#include <stdlib.h>

namespace gloop {

  template<typename T>
  struct RefCounter {
    typedef T *ptr_type;
    typedef T ref_type;

    static void incref(T * const &t) { t->incref(); }
    static void decref(T * const &t) { t->decref(); }
    static T &deref(T * const &t) { return *t; }
  };

  class RefObj {
    mutable int __refcount;

    RefObj(const RefObj &);
    RefObj &operator=(const RefObj &);

  protected:
    virtual ~RefObj() {
    }

  public:
    void incref() const {
      __refcount++;
    }
    void decref() const {
      --__refcount;
      if (!__refcount) {
        delete this;
      }
    }

    int refcount() const {
      return __refcount;
    }
    RefObj() : __refcount(0) {
    }
  };

  class MonitoredRefObj {
    mutable int __refcount;

    MonitoredRefObj(const MonitoredRefObj &);
    MonitoredRefObj &operator=(const MonitoredRefObj &);

  protected:
    virtual void refcountIncreased(int refcount) const {
    }
    virtual void refcountDecreased(int refcount) const {
    }

    virtual ~MonitoredRefObj() {
    }

  public:
    void incref() const {
      __refcount++;
      refcountIncreased(__refcount);
    }
    void decref() const {
      // refcountDecreased() could potentially cause other changes to
      // the refcount, so after the initial decrease, we keep a copy of
      // the value so the object can only be deleted in one place.

      // XXX: if the refcount drops to zero, it can't be rescued inside
      // refcountDecreased. This isn't a problem for what we initially
      // want to use MonitoredMonitoredRefObj for, whereas the above scenario is.
      int r = --__refcount;
      refcountDecreased(r);
      if (!r) {
        delete this;
      }
    }

    int refcount() const {
      return __refcount;
    }
    MonitoredRefObj() : __refcount(0) {
    }
  };

  #ifdef __OBJC__
  #import <Foundation/Foundation.h>

  template<>
  struct RefCounter<id> {
    typedef id ptr_type;
    typedef id ref_type;

    static void incref(id const &t) { [t retain]; }
    static void decref(id const &t) { [t release]; }
    static id deref(id const &t) { return const_cast<id>(t); }
  };

  #endif

  template<typename T, typename R = RefCounter<T> >
  class Ref {
    typename R::ptr_type pointee;
  public:
    Ref() : pointee(NULL) {
    }
    template<typename U, typename S>
    Ref(const Ref<U, S> &p) : pointee(NULL) {
      *this = p;
    }
    Ref(const Ref &p) : pointee(NULL) {
      *this = p;
    }
    Ref(typename R::ptr_type p) : pointee(p) {
      if (pointee) R::incref(pointee);
    }
    ~Ref() {
      if (pointee) R::decref(pointee);
    }

    inline bool operator<(const Ref &rhs) const {
      return pointee < rhs.pointee;
    }

    template<typename U, typename S>
    Ref &operator=(const Ref<U, S> &p) {
      if (p.ptr()) S::incref(p.ptr());
      if (pointee) R::decref(pointee);
      pointee = p.ptr();
      return *this;
    }
    Ref &operator=(const Ref &p) {
      if (p.pointee) R::incref(p.pointee);
      if (pointee) R::decref(pointee);
      pointee = p.pointee;
      return *this;
    }
    Ref &operator=(typename R::ptr_type p) {
      if (p) R::incref(p);
      if (pointee) R::decref(pointee);
      pointee = p;
      return *this;
    }

    typename R::ref_type &operator*() const { return R::deref(pointee); }

    typename R::ptr_type operator->() const { return pointee; }

    typename R::ptr_type ptr() const { return pointee; }

    int refcount() const { return pointee->refcount(); }

    template<typename U, typename S>
    bool operator==(const Ref<U, S> &r) const { return r.ptr() == pointee; }
    template<typename U, typename S>
    bool operator!=(const Ref<U, S> &r) const { return r.ptr() != pointee; }

    bool operator==(const typename R::ptr_type r) const { return r == pointee; }
    bool operator!=(const typename R::ptr_type r) const { return r != pointee; }
  };

}
