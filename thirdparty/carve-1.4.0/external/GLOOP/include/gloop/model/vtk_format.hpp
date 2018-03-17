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
  namespace vtk {

    class VtkReader : public stream::model_reader {
      struct point_t { double x, y, z; };

      void emitPoints(const std::vector<point_t> &point_data, const std::string &base);

      bool readASCII(std::istream &in);
      bool readBinary(std::istream &in);
      bool readPlain(std::istream &in);
      bool readXML(std::istream &in);

    public:
      VtkReader() : stream::model_reader() {
      }

      virtual ~VtkReader() {
      }

      virtual bool read(std::istream &in);
    };

    class VtkWriter : public stream::model_writer {
    public:
      enum Fmt { ASCII, BINARY, XML };

    protected:
      Fmt fmt;

    public:
      VtkWriter(Fmt _fmt) : stream::model_writer(), fmt(_fmt) {
      }

      virtual ~VtkWriter() {
      }

      virtual bool write(std::ostream &in);
    };

  }
}
