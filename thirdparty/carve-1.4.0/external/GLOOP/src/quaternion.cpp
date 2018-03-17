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

#include <gloop/quaternion.hpp>
#include <gloop/matrix.hpp>

#include <math.h>
#include <float.h>

namespace gloop {

  QUAT::operator M3() {
    return M3(1 - 2*y*y - 2*z*z,     2*x*y - 2*z*w,     2*x*z + 2*y*w,  
                  2*x*y + 2*z*w, 1 - 2*x*x - 2*z*z,     2*y*z - 2*x*w,  
                  2*x*z - 2*y*w,     2*y*z + 2*x*w, 1 - 2*x*x - 2*y*y);  
  }

  QUAT::operator M4() {
    return M4(1 - 2*y*y - 2*z*z,     2*x*y - 2*z*w,     2*x*z + 2*y*w, 0.0,
                  2*x*y + 2*z*w, 1 - 2*x*x - 2*z*z,     2*y*z - 2*x*w, 0.0,
                  2*x*z - 2*y*w,     2*y*z + 2*x*w, 1 - 2*x*x - 2*y*y, 0.0,
              0.0,               0.0,               0.0,               1.0);
  }

  QUAT QUAT::slerp(const QUAT &a, const QUAT &b, float t) {
    float cosom, s0, s1;
    
    cosom = a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
    
    if ((1.0f + cosom) > FLT_EPSILON) {
      if ((1.0f - cosom) > FLT_EPSILON) {
        float omega = acosf(cosom);
        float sinom = sinf(omega);
        s0 = sinf((1.0f - t) * omega) / sinom;
        s1 = sinf(t * omega) / sinom;
      } else {
        s0 = 1.0f - t;
        s1 = t;
      }

      return QUAT(s0 * a.x + s1 * b.x,
                  s0 * a.y + s1 * b.y,
                  s0 * a.z + s1 * b.z,
                  s0 * a.w + s1 * b.w);
    } else {
      s0 = sinf((1.0f - t) * (float)(M_PI * 0.5));
      s1 = sinf(t * (float)(M_PI * 0.5));
      
      return QUAT(s0 * a.x + s1 * -b.y,
                  s0 * a.y + s1 * +b.x,
                  s0 * a.z + s1 * -b.w,
                  s0 * a.w + s1 * +b.z);
    }
  }

}
