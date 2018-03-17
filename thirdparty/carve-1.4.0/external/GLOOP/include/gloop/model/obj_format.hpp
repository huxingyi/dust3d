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

#include <gloop/model/stream.hpp>
#include <gloop/stringfuncs.hpp>

#include <iostream>
#include <map>

namespace gloop {
  namespace obj {

    // an obj file is an ascii format.
    // comment lines are denoted by #
    // primitives are presented one per line, with \ representing a continuation.
    // the first whitespace delimted token on each line denotes the type of element represented on the line.

    class ObjReader : public stream::model_reader {

      bool readDoubles(std::istream &in,
                       stream::reader_base *e_rd,
                       stream::reader_base **v_rd,
                       size_t min_count,
                       size_t rd_count,
                       const double *defaults);
      bool readIntLists(std::istream &in,
                        stream::reader_base *e_rd,
                        stream::reader_base **v_rd,
                        size_t rd_count);

    public:
      ObjReader() : stream::model_reader() {
      }

      virtual ~ObjReader() {
      }

      virtual bool read(std::istream &in);
    };

    class ObjWriter : public stream::model_writer {

    public:

      ObjWriter() : stream::model_writer() {
      }

      virtual ~ObjWriter() {
      }

      virtual bool write(std::ostream &in);
    };

  }
}
