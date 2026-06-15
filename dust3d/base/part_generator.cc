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

#include <dust3d/base/part_generator.h>

namespace dust3d {

PartGenerator PartGeneratorFromString(const char* generatorString)
{
    std::string generator = generatorString;
    if (generator == "Imported")
        return PartGenerator::Imported;
    if (generator == "Rock")
        return PartGenerator::Rock;
    return PartGenerator::None;
}

const char* PartGeneratorToString(PartGenerator generator)
{
    switch (generator) {
    case PartGenerator::None:
        return "None";
    case PartGenerator::Imported:
        return "Imported";
    case PartGenerator::Rock:
        return "Rock";
    default:
        return "None";
    }
}

std::string PartGeneratorToDispName(PartGenerator generator)
{
    switch (generator) {
    case PartGenerator::None:
        return std::string("None");
    case PartGenerator::Imported:
        return std::string("Imported");
    case PartGenerator::Rock:
        return std::string("Rock");
    default:
        return std::string("None");
    }
}

}
