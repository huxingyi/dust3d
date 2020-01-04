/*
This file is an unofficial API exporter for Instant-Meshes on Windows.

Copyright (c) 2015 Wenzel Jakob, Daniele Panozzo, Marco Tarini,
and Olga Sorkine-Hornung. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

You are under no obligation whatsoever to provide any bug fixes, patches, or
upgrades to the features, functionality or performance of the source code
("Enhancements") to anyone; however, if you choose to make your Enhancements
available either publicly, or directly to the authors of this software, without
imposing a separate written license agreement for such Enhancements, then you
hereby grant the following license: a non-exclusive, royalty-free perpetual
license to install, use, modify, prepare derivative works, incorporate into
other computer software, distribute, and sublicense such enhancements or
derivative works thereof, in binary and source code form.
*/
#ifndef DUST3D_INSTANT_MESHES_API_H
#define DUST3D_INSTANT_MESHES_API_H
#include <stddef.h>

typedef struct
{
    float x;
    float y;
    float z;
} Dust3D_InstantMeshesVertex;

typedef struct
{
    size_t indices[3];
} Dust3D_InstantMeshesTriangle;

typedef struct
{
    size_t indices[4];
} Dust3D_InstantMeshesQuad;

#ifdef _WIN32
#    ifdef DUST3D_INSTANT_MESHES_API_EXPORTS
#        define DUST3D_INSTANT_MESHES_API extern "C" __declspec(dllexport)
#    else
#        define DUST3D_INSTANT_MESHES_API extern "C" __declspec(dllimport)
#    endif
#    define DUST3D_INSTANT_MESHES_FUNCTION_CONVENTION __stdcall
#else
#    define DUST3D_INSTANT_MESHES_FUNCTION_CONVENTION
#    define DUST3D_INSTANT_MESHES_API extern "C"
#endif

DUST3D_INSTANT_MESHES_API void DUST3D_INSTANT_MESHES_FUNCTION_CONVENTION Dust3D_instantMeshesRemesh(const Dust3D_InstantMeshesVertex *vertices, size_t nVertices,
    const Dust3D_InstantMeshesTriangle *triangles, size_t nTriangles,
    size_t nTargetVertex,
    const Dust3D_InstantMeshesVertex **resultVertices,
    size_t *nResultVertices,
    const Dust3D_InstantMeshesTriangle **resultTriangles,
    size_t *nResultTriangles,
    const Dust3D_InstantMeshesQuad **resultQuads,
    size_t *nResultQuads);

#endif
