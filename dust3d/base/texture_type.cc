/*
 *  Copyright (c) 2016-2021 Jeremy HU <jeremy-at-dust3d dot org>. All rights reserved. 
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

#include <dust3d/base/texture_type.h>

namespace dust3d {

const char* TextureTypeToString(TextureType type)
{
    switch (type) {
    case TextureType::BaseColor:
        return "BaseColor";
    case TextureType::Normal:
        return "Normal";
    case TextureType::Metallic:
        return "Metallic";
    case TextureType::Roughness:
        return "Roughness";
    case TextureType::AmbientOcclusion:
        return "AmbientOcclusion";
    case TextureType::None:
        return "None";
    default:
        return "";
    }
}

TextureType TextureTypeFromString(const char* typeString)
{
    std::string type = typeString;
    if (type == "BaseColor")
        return TextureType::BaseColor;
    if (type == "Normal")
        return TextureType::Normal;
    if (type == "Metallic")
        return TextureType::Metallic;
    if (type == "Metalness")
        return TextureType::Metallic;
    if (type == "Roughness")
        return TextureType::Roughness;
    if (type == "AmbientOcclusion")
        return TextureType::AmbientOcclusion;
    return TextureType::None;
}

std::string TextureTypeToDispName(TextureType type)
{
    switch (type) {
    case TextureType::BaseColor:
        return std::string("Base Color");
    case TextureType::Normal:
        return std::string("Normal Map");
    case TextureType::Metallic:
        return std::string("Metallic");
    case TextureType::Roughness:
        return std::string("Roughness");
    case TextureType::AmbientOcclusion:
        return std::string("Ambient Occlusion");
    case TextureType::None:
        return std::string("None");
    default:
        return "";
    }
}

}
