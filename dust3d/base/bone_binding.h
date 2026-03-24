/*
 *  Copyright (c) 2016-2026 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:

 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.

 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#ifndef DUST3D_BASE_BONE_BINDING_H_
#define DUST3D_BASE_BONE_BINDING_H_

#include <string>

namespace dust3d {

// Vertex bone binding: stores bone influences for a single vertex
// A vertex can be influenced by at most 2 bones with linear interpolation
struct VertexBoneBinding {
    std::string bone1; // Primary bone name
    float weight1 = 1.0f; // Weight for bone1 (0.0 to 1.0)
    std::string bone2; // Secondary bone name (empty if single bone)
    float weight2 = 0.0f; // Weight for bone2 (should be 1.0 - weight1 if bone2 exists)

    VertexBoneBinding() = default;
    VertexBoneBinding(const std::string& b1, float w1 = 1.0f, const std::string& b2 = std::string(), float w2 = 0.0f)
        : bone1(b1)
        , weight1(w1)
        , bone2(b2)
        , weight2(w2)
    {
    }

    bool isValid() const { return !bone1.empty(); }
    bool isSingleBone() const { return bone2.empty(); }
};

// Node bone influence: stores which bones a node is influenced by
// A node can be influenced by at most 2 bones based on its connected edges
struct NodeBoneInfluence {
    std::string primary; // Primary bone name (or only bone if single)
    std::string secondary; // Secondary bone name (empty if single)
    float lerp = 0.0f; // Interpolation factor between primary and secondary (0.0 = all primary, 1.0 = all secondary)

    NodeBoneInfluence() = default;
    NodeBoneInfluence(const std::string& p, const std::string& s = std::string(), float l = 0.0f)
        : primary(p)
        , secondary(s)
        , lerp(l)
    {
    }

    bool isValid() const { return !primary.empty(); }
    bool isSingleBone() const { return secondary.empty(); }

    // Convert node influence to vertex binding
    VertexBoneBinding toVertexBinding() const
    {
        if (isSingleBone()) {
            return VertexBoneBinding(primary, 1.0f);
        } else {
            float w1 = 1.0f - lerp;
            float w2 = lerp;
            return VertexBoneBinding(primary, w1, secondary, w2);
        }
    }
};

}

#endif
