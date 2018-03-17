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

#include <gloop/exceptions.hpp>
#include <gloop/ref.hpp>

#ifdef WIN32

typedef char int8_t;
typedef short int16_t;
typedef long int32_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

#else 

#include <stdint.h>

#endif

#include <stdlib.h>

#include <string>
#include <list>
#include <vector>

namespace gloop {
  namespace stream {

    enum Type {
      I8  = 0x00,
      U8  = 0x01,
      I16 = 0x02,
      U16 = 0x03,
      I32 = 0x04,
      U32 = 0x05,
      F32 = 0x06,
      F64 = 0x07
    };

    static inline Type smallest_type(int32_t min, int32_t max) {
      if (max < 0x80   && min >= -0x80)   return I8;
      if (max < 0x8000 && min >= -0x8000) return I16;
      return I32;
    }

    static inline Type smallest_type(uint32_t max) {
      if (max < 0x100)   return U8;
      if (max < 0x10000) return U16;
      return U32;
    }

    static inline int type_size(int type) {
      switch (type) {
      case I8:  case U8:            return 1;
      case I16: case U16:           return 2;
      case I32: case U32: case F32: return 4;
      case F64:                     return 8;
      default: throw gloop::exception("unknown type");
      }
    }



    struct reader_base : public virtual RefObj {
      virtual void _val(int8_t t)   =0;
      virtual void _val(uint8_t t)  =0;
      virtual void _val(int16_t t)  =0;
      virtual void _val(uint16_t t) =0;
      virtual void _val(int32_t t)  =0;
      virtual void _val(uint32_t t) =0;
      virtual void _val(float t)    =0;
      virtual void _val(double t)   =0;

      virtual void begin() { }
      virtual void next() { }
      virtual void fail() { }
      virtual void end() { }

      virtual void length(int l) { }
    };



    struct null_reader : public reader_base {
      virtual void _val(int8_t t)   { }
      virtual void _val(uint8_t t)  { }
      virtual void _val(int16_t t)  { }
      virtual void _val(uint16_t t) { }
      virtual void _val(int32_t t)  { }
      virtual void _val(uint32_t t) { }
      virtual void _val(float t)    { }
      virtual void _val(double t)   { }
    };



    template<typename T>
    struct reader : public reader_base {
      virtual void value(T val) =0;
      virtual void _val(int8_t t)   { value((T)t); }
      virtual void _val(uint8_t t)  { value((T)t); }
      virtual void _val(int16_t t)  { value((T)t); }
      virtual void _val(uint16_t t) { value((T)t); }
      virtual void _val(int32_t t)  { value((T)t); }
      virtual void _val(uint32_t t) { value((T)t); }
      virtual void _val(float t)    { value((T)t); }
      virtual void _val(double t)   { value((T)t); }
    };

    template<typename T>
    void send_scalar(reader_base *rd, const T &v) {
      if (rd) {
        rd->begin();
        rd->length(1);
        rd->next();
        rd->_val(v);
        rd->end();
      }
    }

    template<typename iter_t>
    void send_sequence(reader_base *rd, size_t n, iter_t begin, iter_t end) {
      if (rd) {
        rd->begin();
        rd->length(n);
        while (begin != end) {
          rd->next();
          rd->_val(*begin++);
        }
        rd->end();
      }
    }

    struct writer_base : public virtual RefObj {
      virtual void _val(int8_t &t)   =0;
      virtual void _val(uint8_t &t)  =0;
      virtual void _val(int16_t &t)  =0;
      virtual void _val(uint16_t &t) =0;
      virtual void _val(int32_t &t)  =0;
      virtual void _val(uint32_t &t) =0;
      virtual void _val(float &t)    =0;
      virtual void _val(double &t)   =0;

      virtual bool isList() { return false; }
      virtual Type dataType() { return F64; }

      virtual void begin() { }
      virtual void next() { }
      virtual void fail() { }
      virtual void end() { }

      virtual int length() { return 0; }
      virtual int maxLength() { return 0; }
    };



    struct null_writer : public writer_base {
      virtual void _val(int8_t &t)   { }
      virtual void _val(uint8_t &t)  { }
      virtual void _val(int16_t &t)  { }
      virtual void _val(uint16_t &t) { }
      virtual void _val(int32_t &t)  { }
      virtual void _val(uint32_t &t) { }
      virtual void _val(float &t)    { }
      virtual void _val(double &t)   { }
    };



    template<typename T>
    struct writer : public writer_base {
      virtual T value() =0;
      virtual void _val(int8_t &t)   { t = (int8_t)value(); }
      virtual void _val(uint8_t &t)  { t = (uint8_t)value(); }
      virtual void _val(int16_t &t)  { t = (int16_t)value(); }
      virtual void _val(uint16_t &t) { t = (uint16_t)value(); }
      virtual void _val(int32_t &t)  { t = (int32_t)value(); }
      virtual void _val(uint32_t &t) { t = (uint32_t)value(); }
      virtual void _val(float &t)    { t = (float)value(); }
      virtual void _val(double &t)   { t = (double)value(); }
    };

    // a specification string has the form:
    //   block_type.element.property

    // for example:
    //   polyhedron.face.vertex_indices

    // for readers, the ordering of specifications is unimportant.

    // for writers, within a block there should be a single block type.

    // within that block, specifications should be ordered by order of
    // addition of element and then by order of addition of property.

    struct named_prop_t {
      std::string name;

      Ref<reader_base> rd;
      Ref<writer_base> wt;

      named_prop_t(const std::string &n) : name(n), rd(NULL), wt(NULL) {}
    };

    typedef std::list<named_prop_t> named_prop_list_t;

    struct named_element_t {
      std::string name;
      named_prop_list_t props;

      Ref<reader_base> rd;
      Ref<writer_base> wt;

      named_element_t(const std::string &n) : name(n), props(), rd(NULL), wt(NULL) {}

      named_prop_t *findProp(const std::string &n) const;
      named_prop_t &findOrCreateProp(const std::string &n);
    };

    typedef std::list<named_element_t> named_element_list_t;

    struct block_t {
      std::string name;
      named_element_list_t elems;

      Ref<reader_base> rd;
      Ref<writer_base> wt;

      block_t(const std::string &n) : name(n), elems(), rd(NULL), wt(NULL) {}

      named_element_t *findElem(const std::string &n) const;
      named_element_t &findOrCreateElem(const std::string &n);
    };

    typedef std::list<block_t> block_list_t;


    class model_reader {
    public:
      block_list_t blocks;

      block_t *findBlock(const std::string &n) const;
      block_t &findOrCreateBlock(const std::string &n);

      named_element_t *findElem(const std::string &b, const std::string &e) const;
      named_element_t &findOrCreateElem(const std::string &b, const std::string &e);

      named_prop_t *findProp(const std::string &b, const std::string &e, const std::string &p) const;
      named_prop_t &findOrCreateProp(const std::string &b, const std::string &e, const std::string &p);

      reader_base *findReader(const std::string &b, const std::string &e, const std::string &p) const {
        named_prop_t *prop = findProp(b, e, p); if (prop) return prop->rd.ptr();
        return NULL;
      }
      reader_base *findReader(const std::string &b, const std::string &e) const {
        named_element_t *elem = findElem(b, e); if (elem) return elem->rd.ptr();
        return NULL;
      }
      reader_base *findReader(const std::string &b) const {
        block_t *blk = findBlock(b); if (blk) return blk->rd.ptr();
        return NULL;
      }

      model_reader() {
      }

      virtual ~model_reader() {
      }

      bool addReader(const std::string &spec, reader_base *rd);

      virtual bool read(std::istream &in) =0;
    };


    struct model_writer {
      std::list<std::pair<std::string, block_t> > blocks;
    public:
      model_writer() {
      }

      virtual ~model_writer() {
      }

      bool newBlock(const std::string &name);
      bool addWriter(const std::string &spec, writer_base *wt);

      virtual bool write(std::ostream &out) =0;
    };

  }
}
