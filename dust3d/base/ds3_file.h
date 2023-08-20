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

#ifndef DUST3D_BASE_DS3_FILE_H_
#define DUST3D_BASE_DS3_FILE_H_

#include <map>
#include <string>
#include <vector>

namespace dust3d {

class Ds3ReaderItem {
public:
    std::string type;
    std::string name;
    long long offset;
    long long size;
};

class Ds3FileReader {
public:
    Ds3FileReader(const std::uint8_t* fileData, size_t fileSize);
    void loadItem(const std::string& name, std::vector<std::uint8_t>* byteArray);
    const std::vector<Ds3ReaderItem>& items() const;
    static std::string m_applicationName;
    static std::string m_magicApplicationName;
    static std::string m_fileFormatVersion;
    static std::string m_headFormat;

private:
    std::map<std::string, Ds3ReaderItem> m_itemsMap;
    std::vector<Ds3ReaderItem> m_items;
    std::vector<std::uint8_t> m_fileContent;

private:
    static std::string readFirstLine(const std::uint8_t* data, size_t size);
    bool m_headerIsGood = false;
    long long m_binaryOffset = 0;
};

class Ds3WriterItem {
public:
    std::string type;
    std::string name;
    std::vector<std::uint8_t> byteArray;
};

class Ds3FileWriter {
public:
    bool add(const std::string& name, const std::string& type, const void* buffer, size_t bufferSize);
    bool save(const std::string& filename);
    void save(std::vector<std::uint8_t>& byteArray);

private:
    std::map<std::string, Ds3WriterItem> m_itemsMap;
    std::vector<Ds3WriterItem> m_items;
    std::string m_filename;
    void getHeaderXml(std::string& headerXml);
};

}

#endif
