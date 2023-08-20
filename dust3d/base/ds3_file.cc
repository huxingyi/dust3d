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

#include <cstring>
#include <dust3d/base/debug.h>
#include <dust3d/base/ds3_file.h>
#include <dust3d/base/string.h>
#include <fstream>
#include <iostream>
#include <rapidxml.hpp>

namespace dust3d {

std::string Ds3FileReader::m_applicationName = std::string("DUST3D");
std::string Ds3FileReader::m_magicApplicationName = char(0xd3) + (char(0x3d) + std::string("DUST3D"));
std::string Ds3FileReader::m_fileFormatVersion = std::string("1.0");
std::string Ds3FileReader::m_headFormat = std::string("xml");

std::string Ds3FileReader::readFirstLine(const std::uint8_t* data, size_t size)
{
    std::string firstLine;
    for (size_t i = 0; i < size; ++i) {
        if ('\n' == (char)data[i]) {
            return firstLine;
        }
        firstLine += (char)data[i];
    }
    return std::string();
}

Ds3FileReader::Ds3FileReader(const std::uint8_t* fileData, size_t fileSize)
{
    m_headerIsGood = false;
    std::string firstLine = readFirstLine(fileData, fileSize);
    std::vector<std::string> tokens = String::split(firstLine, ' ');
    if (tokens.size() < 4) {
        dust3dDebug << "Unexpected file header";
        return;
    }
    if (tokens[0] != Ds3FileReader::m_magicApplicationName && tokens[0] != Ds3FileReader::m_applicationName) {
        dust3dDebug << "Unrecognized application name:" << tokens[0];
        return;
    }
    if (tokens[1] != Ds3FileReader::m_fileFormatVersion) {
        dust3dDebug << "Unrecognized file format version:" << tokens[1];
        return;
    }
    if (tokens[2] != Ds3FileReader::m_headFormat) {
        dust3dDebug << "Unrecognized file head format:" << tokens[2];
        return;
    }
    m_binaryOffset = std::stoull(tokens[3]);
    if (m_binaryOffset > (long long)fileSize) {
        m_binaryOffset = 0;
        return;
    }
    std::vector<char> header(m_binaryOffset - firstLine.size() + 1);
    std::memcpy(header.data(), fileData + firstLine.size(), header.size() - 1);
    header[header.size() - 1] = '\0';

    try {
        rapidxml::xml_document<> xml;
        xml.parse<0>(header.data());
        rapidxml::xml_node<>* rootNode = xml.first_node("ds3");
        if (nullptr == rootNode)
            return;
        m_headerIsGood = true;
        m_fileContent = std::vector<std::uint8_t>(fileSize);
        std::memcpy(m_fileContent.data(), fileData, fileSize);
        for (rapidxml::xml_node<>* node = rootNode->first_node(); nullptr != node; node = node->next_sibling()) {
            Ds3ReaderItem readerItem;
            rapidxml::xml_attribute<>* attribute;
            readerItem.type = node->name();
            if (nullptr != (attribute = node->first_attribute("name")))
                readerItem.name = String::trimmed(attribute->value());
            if (nullptr != (attribute = node->first_attribute("offset")))
                readerItem.offset = std::stoull(attribute->value());
            if (nullptr != (attribute = node->first_attribute("size")))
                readerItem.size = std::stoull(attribute->value());
            if (readerItem.offset > (long long)fileSize)
                continue;
            if (readerItem.offset + readerItem.size > (long long)fileSize)
                continue;
            m_items.push_back(readerItem);
            m_itemsMap[readerItem.name] = readerItem;
        }
    } catch (const std::runtime_error& e) {
        dust3dDebug << "Runtime error was: " << e.what();
    } catch (const rapidxml::parse_error& e) {
        dust3dDebug << "Parse error was: " << e.what();
    } catch (const std::exception& e) {
        dust3dDebug << "Error was: " << e.what();
    } catch (...) {
        dust3dDebug << "An unknown error occurred.";
    }
}

void Ds3FileReader::loadItem(const std::string& name, std::vector<std::uint8_t>* byteArray)
{
    byteArray->clear();
    if (!m_headerIsGood)
        return;
    if (m_itemsMap.find(name) == m_itemsMap.end()) {
        return;
    }
    Ds3ReaderItem readerItem = m_itemsMap[name];
    byteArray->resize(readerItem.size, 0);
    std::memcpy((char*)byteArray->data(),
        m_fileContent.data() + m_binaryOffset + readerItem.offset,
        byteArray->size());
}

const std::vector<Ds3ReaderItem>& Ds3FileReader::items() const
{
    return m_items;
}

bool Ds3FileWriter::add(const std::string& name, const std::string& type, const void* buffer, size_t bufferSize)
{
    if (m_itemsMap.find(name) != m_itemsMap.end()) {
        return false;
    }
    Ds3WriterItem writerItem;
    writerItem.type = type;
    writerItem.name = name;
    writerItem.byteArray.resize(bufferSize);
    for (size_t i = 0; i < bufferSize; ++i)
        writerItem.byteArray[i] = ((const std::uint8_t*)buffer)[i];
    m_itemsMap[name] = writerItem;
    m_items.push_back(writerItem);
    return true;
}

void Ds3FileWriter::getHeaderXml(std::string& headerXml)
{
    std::ostringstream headerXmlStream;
    headerXmlStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    headerXmlStream << "<ds3>" << std::endl;
    {
        long long offset = 0;
        for (size_t i = 0; i < m_items.size(); i++) {
            Ds3WriterItem* writerItem = &m_items[i];
            headerXmlStream << "    <" << writerItem->type;
            headerXmlStream << " name=\"" << String::doubleQuoteEscapedForXml(writerItem->name) << "\"";
            headerXmlStream << " offset=\"" << std::to_string(offset) << "\"";
            headerXmlStream << " size=\"" << std::to_string(writerItem->byteArray.size()) << "\"";
            offset += writerItem->byteArray.size();
            headerXmlStream << "/>" << std::endl;
        }
    }
    headerXmlStream << "</ds3>" << std::endl;
    headerXml = headerXmlStream.str();
}

bool Ds3FileWriter::save(const std::string& filename)
{
    std::ofstream file(filename, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!file.is_open())
        return false;

    std::string headerXml;
    getHeaderXml(headerXml);

    char firstLine[1024];
    int firstLineSizeExcludeSizeSelf = sprintf(firstLine, "%s %s %s ",
        Ds3FileReader::m_magicApplicationName.c_str(),
        Ds3FileReader::m_fileFormatVersion.c_str(),
        Ds3FileReader::m_headFormat.c_str());
    unsigned int headerSize = (unsigned int)(firstLineSizeExcludeSizeSelf + 12 + headerXml.size());
    char headerSizeString[100] = { 0 };
    sprintf(headerSizeString, "%010u\r\n", headerSize);
    file.write(firstLine, firstLineSizeExcludeSizeSelf);
    file.write(headerSizeString, strlen(headerSizeString));
    file << headerXml;
    for (size_t i = 0; i < m_items.size(); i++) {
        Ds3WriterItem* writerItem = &m_items[i];
        file.write((char*)writerItem->byteArray.data(), writerItem->byteArray.size());
    }

    return true;
}

void Ds3FileWriter::save(std::vector<std::uint8_t>& byteArray)
{
    std::string headerXml;
    getHeaderXml(headerXml);

    char firstLine[1024];
    int firstLineSizeExcludeSizeSelf = sprintf(firstLine, "%s %s %s ",
        Ds3FileReader::m_magicApplicationName.c_str(),
        Ds3FileReader::m_fileFormatVersion.c_str(),
        Ds3FileReader::m_headFormat.c_str());
    unsigned int headerSize = (unsigned int)(firstLineSizeExcludeSizeSelf + 12 + headerXml.size());
    char headerSizeString[100] = { 0 };
    sprintf(headerSizeString, "%010u\r\n", headerSize);

    byteArray.insert(byteArray.end(), firstLine, firstLine + firstLineSizeExcludeSizeSelf);
    byteArray.insert(byteArray.end(), headerSizeString, headerSizeString + strlen(headerSizeString));
    std::copy(headerXml.begin(), headerXml.end(), std::back_inserter(byteArray));
    for (size_t i = 0; i < m_items.size(); i++) {
        Ds3WriterItem* writerItem = &m_items[i];
        byteArray.insert(byteArray.end(), (char*)writerItem->byteArray.data(), (char*)writerItem->byteArray.data() + writerItem->byteArray.size());
    }
}

}
