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

#ifndef DUST3D_BASE_STRING_H_
#define DUST3D_BASE_STRING_H_

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace dust3d {
namespace String {

    inline std::vector<std::string> split(const std::string& string, char seperator)
    {
        std::vector<std::string> tokens;
        std::stringstream stream(string);
        std::string token;
        while (std::getline(stream, token, seperator))
            tokens.push_back(token);
        return tokens;
    }

    inline std::string& trimLeft(std::string& string)
    {
        string.erase(string.begin(), std::find_if(string.begin(), string.end(), [](int ch) {
            return !std::isspace(ch);
        }));
        return string;
    }

    inline std::string& trimRight(std::string& string)
    {
        string.erase(std::find_if(string.rbegin(), string.rend(), [](int ch) {
            return !std::isspace(ch);
        }).base(),
            string.end());
        return string;
    }

    inline std::string trimmed(std::string string)
    {
        trimLeft(string);
        trimRight(string);
        return string;
    }

    inline std::string doubleQuoteEscapedForXml(const std::string& string)
    {
        std::string escapedString;
        escapedString.reserve(string.size() + 1);
        for (std::string::size_type i = 0; i < string.size(); ++i) {
            char c = string[i];
            if ('"' != c) {
                escapedString += c;
                continue;
            }
            escapedString += "&quot;";
        }
        return escapedString;
    }

    inline float toFloat(const std::string& string)
    {
        return (float)std::stod(string);
    }

    std::string join(const std::vector<std::string>& stringList, const char* separator);

    inline std::string valueOrEmpty(const std::map<std::string, std::string>& map, const std::string& key)
    {
        auto it = map.find(key);
        if (it == map.end())
            return std::string();
        return it->second;
    }

    inline bool isTrue(const std::string& string)
    {
        return "true" == string || "True" == string || "1" == string;
    }

    inline bool startsWith(const std::string& string, const std::string searchItem)
    {
        return string.rfind(searchItem, 0) == 0;
    }

}
}

#endif
