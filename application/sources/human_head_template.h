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

#ifndef DUST3D_APPLICATION_HUMAN_HEAD_TEMPLATE_H_
#define DUST3D_APPLICATION_HUMAN_HEAD_TEMPLATE_H_

#include <dust3d/base/snapshot.h>
#include <dust3d/base/uuid.h>

// The built-in human head, to start a character's head from: the head (an imported model, the
// ICT FaceKit neutral face) in a group with its face painted on it: the eyes, a line along
// each upper lid, the brows and the lips, each an ordinary paint part to recolour or move.
// Hair is drawn on it with Hair parts.
//
// The head stands with its chin `height` below its crown and the middle of it at
// (x, y, z) in the document's world (the canvas origin is 0). Canvas coordinates come from
// the origin given. Returns false when the bundled model is missing.
bool makeHumanHeadSnapshot(dust3d::Snapshot* snapshot, float originX, float originY, float originZ,
    float x, float y, float z, float height);

#endif
