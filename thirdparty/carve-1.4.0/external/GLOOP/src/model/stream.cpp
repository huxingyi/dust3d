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

#include <gloop/model/stream.hpp>
#include <gloop/stringfuncs.hpp>

#ifdef WIN32

#include <stdarg.h>

#endif

namespace gloop {
  namespace stream {

    named_prop_t *named_element_t::findProp(const std::string &n) const {
      for (named_prop_list_t::const_iterator i = props.begin(); i != props.end(); ++i) {
        if ((*i).name == n) return const_cast<named_prop_t *>(&*i);
      }
      return NULL;
    }

    named_prop_t &named_element_t::findOrCreateProp(const std::string &n) {
      named_prop_t *p = findProp(n);
      if (p == NULL) {
        props.push_back(named_prop_t(n));
        p = &props.back();
      }
      return *p;
    }


    named_element_t *block_t::findElem(const std::string &n) const {
      for (named_element_list_t::const_iterator i = elems.begin(); i != elems.end(); ++i) {
        if ((*i).name == n) return const_cast<named_element_t *>(&*i);
      }
      return NULL;
    }

    named_element_t &block_t::findOrCreateElem(const std::string &n) {
      named_element_t *e = findElem(n);
      if (e == NULL) {
        elems.push_back(named_element_t(n));
        e = &elems.back();
      }
      return *e;
    }


    block_t *model_reader::findBlock(const std::string &n) const {
      for (std::list<block_t>::const_iterator i = blocks.begin(); i != blocks.end(); ++i) {
        if ((*i).name == n) return const_cast<block_t *>(&*i);
      }
      return NULL;
    }

    block_t &model_reader::findOrCreateBlock(const std::string &n) {
      block_t *b = findBlock(n);
      if (b == NULL) {
        blocks.push_back(block_t(n));
        b = &blocks.back();
      }
      return *b;
    }

    named_element_t *model_reader::findElem(const std::string &b, const std::string &e) const {
      block_t *block = findBlock(b);
      if (!block) return NULL;
      return block->findElem(e);
    }
    named_element_t &model_reader::findOrCreateElem(const std::string &b, const std::string &e) {
      return findOrCreateBlock(b).findOrCreateElem(e);
    }

    named_prop_t *model_reader::findProp(const std::string &b, const std::string &e, const std::string &p) const {
      block_t *block = findBlock(b);
      if (!block) return NULL;
      named_element_t *elem = block->findElem(e);
      if (!elem) return NULL;
      return elem->findProp(p);
    }
    named_prop_t &model_reader::findOrCreateProp(const std::string &b, const std::string &e, const std::string &p) {
      return findOrCreateBlock(b).findOrCreateElem(e).findOrCreateProp(p);
    }

    bool model_reader::addReader(const std::string &spec, reader_base *rd) {
      std::vector<std::string> splitspec;
      str::split(std::back_inserter(splitspec), spec, '.', 2);

//       std::cerr << "addReader(";
//       for (size_t i = 0; i < splitspec.size(); ++i) {
//         std::cerr << "[" << splitspec[i] << "]";
//       }
//       std::cerr << ")=" << rd << std::endl;

      switch(splitspec.size()) {
      case 1: {
        findOrCreateBlock(splitspec[0]).rd = rd;
        return true;
      }
      case 2: {
        findOrCreateElem(splitspec[0], splitspec[1]).rd = rd;
        return true;
      }
      case 3: {
        findOrCreateProp(splitspec[0], splitspec[1], splitspec[2]).rd = rd;
        return true;
      }
      default: {
        return false;
      }
      }
    }


    bool model_writer::newBlock(const std::string &name) {
      blocks.push_back(std::make_pair(name, block_t("")));
      return true;
    }

    bool model_writer::addWriter(const std::string &spec, writer_base *wt) {
      if (!blocks.size()) return false;
      std::vector<std::string> splitspec;
      str::split(std::back_inserter(splitspec), spec, '.', 2);

      block_t &block(blocks.back().second);
      if (splitspec.size() < 1) return false;
      if (block.name != "" && block.name != splitspec[0]) return false;
      block.name = splitspec[0];
      switch(splitspec.size()) {
      case 1: {
        block.wt = wt;
        return true;
      }
      case 2: {
        block.findOrCreateElem(splitspec[1]).wt = wt;
        return true;
      }
      case 3: {
        block.findOrCreateElem(splitspec[1]).findOrCreateProp(splitspec[2]).wt = wt;
        return true;
      }
      default: {
        return false;
      }
      }
    }

  }
}
