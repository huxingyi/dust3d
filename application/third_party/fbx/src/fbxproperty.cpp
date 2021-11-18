#include "fbxproperty.h"
#include "fbxutil.h"
#include <functional>
// Change to miniz in Dust3D project
#include <miniz.h>

using std::cout;
using std::endl;
using std::string;

namespace fbx {

namespace { // helpers for reading properties
    FBXPropertyValue readPrimitiveValue(Reader &reader, char type)
    {
        FBXPropertyValue value;
        if(type == 'Y') { // 2 byte signed integer
            value.i16 = reader.readInt16();
        } else if(type == 'C' || type == 'B') { // 1 bit boolean (1: true, 0: false) encoded as the LSB of a 1 Byte value.
            value.boolean = reader.readUint8() != 0;
        } else if(type == 'I') { // 4 byte signed Integer
            value.i32 = reader.readInt32();
        } else if(type == 'F') { // 4 byte single-precision IEEE 754 number
            value.f32 = reader.readFloat();
        } else if(type == 'D') { // 8 byte double-precision IEEE 754 number
            value.f64 = reader.readDouble();
        } else if(type == 'L') { // 8 byte signed Integer
            value.i64 = reader.readUint64();
        } else {
            throw std::string("Unsupported property type ")+std::to_string(type);
        }
        return value;
    }

    uint32_t arrayElementSize(char type)
    {
        if(type == 'Y') { // 2 byte signed integer
            return 2;
        } else if(type == 'C' || type == 'B') { // 1 bit boolean (1: true, 0: false) encoded as the LSB of a 1 Byte value.
            return 1;
        } else if(type == 'I') { // 4 byte signed Integer
            return 4;
        } else if(type == 'F') { // 4 byte single-precision IEEE 754 number
            return 4;
        } else if(type == 'D') { // 8 byte double-precision IEEE 754 number
            return 8;
        } else if(type == 'L') { // 8 byte signed Integer
            return 8;
        } else {
            return 0;
        }
    }

    class STRMAutoCloser
    {
    public:
        z_stream *strm;
        STRMAutoCloser(z_stream& _strm):strm(&_strm) {}

        ~STRMAutoCloser() {
            (void)inflateEnd(strm);
        }
    };

    class BufferAutoFree
    {
    public:
        uint8_t *buffer;
        BufferAutoFree(uint8_t *buf):buffer(buf) {}

        ~BufferAutoFree() {
            free(buffer);
        }
    };
}

FBXProperty::FBXProperty(std::ifstream &input)
{
    Reader reader(&input);
    type = reader.readUint8();
    // std::cout << "  " << type << "\n";
    if(type == 'S' || type == 'R') {
        uint32_t length = reader.readUint32();
        for(uint32_t i = 0; i < length; i++){
            uint8_t v = reader.readUint8();
            raw.push_back(v);
        }
    } else if(type < 'Z') { // primitive types
        value = readPrimitiveValue(reader, type);
    } else {
        uint32_t arrayLength = reader.readUint32(); // number of elements in array
        uint32_t encoding = reader.readUint32(); // 0 .. uncompressed, 1 .. zlib-compressed
        uint32_t compressedLength = reader.readUint32();
        if(encoding) {
            uint64_t uncompressedLength = arrayElementSize(type - ('a'-'A')) * arrayLength;

            uint8_t *decompressedBuffer = (uint8_t*) malloc(uncompressedLength);
            if(decompressedBuffer == NULL) throw std::string("Malloc failed");
            BufferAutoFree baf(decompressedBuffer);

            std::vector<uint8_t> compressedBufferVector(compressedLength);
            uint8_t *compressedBuffer = compressedBufferVector.data();
            reader.read((char*)compressedBuffer, compressedLength);

            mz_ulong destLen = uncompressedLength;
            mz_ulong srcLen = compressedLength;
            mz_uncompress(decompressedBuffer, &destLen, compressedBuffer, srcLen);

            if(srcLen != compressedLength) throw std::string("compressedLength does not match data");
            if(destLen != uncompressedLength) throw std::string("uncompressedLength does not match data");

            Reader r((char*)decompressedBuffer);

            for(uint32_t i = 0; i < arrayLength; i++) {
                values.push_back(readPrimitiveValue(r, type - ('a'-'A')));
            }
        } else {
            for(uint32_t i = 0; i < arrayLength; i++) {
                values.push_back(readPrimitiveValue(reader, type - ('a'-'A')));
            }
        }
    }
}

void FBXProperty::write(std::ofstream &output)
{
    Writer writer(&output);

    writer.write(type);
    if(type == 'Y') {
        writer.write(value.i16);
    } else if(type == 'C') {
        writer.write((uint8_t)(value.boolean ? 1 : 0));
    } else if(type == 'I') {
        writer.write(value.i32);
    } else if(type == 'F') {
        writer.write(value.f32);
    } else if(type == 'D') {
        writer.write(value.f64);
    } else if(type == 'L') {
        writer.write(value.i64);
    } else if(type == 'R' || type == 'S') {
        writer.write((uint32_t)raw.size());
        for(char c : raw) {
            writer.write((uint8_t)c);
        }
    } else {
        writer.write((uint32_t) values.size()); // arrayLength
        writer.write((uint32_t) 0); // encoding // TODO: support compression
        uint32_t compressedLength = 0;
        if(type == 'f') compressedLength = values.size() * 4;
        else if(type == 'd') compressedLength = values.size() * 8;
        else if(type == 'l') compressedLength = values.size() * 8;
        else if(type == 'i') compressedLength = values.size() * 4;
        else if(type == 'b') compressedLength = values.size() * 1;
        else throw std::string("Invalid property");
        writer.write(compressedLength);

        for(auto e : values) {
            if(type == 'f') writer.write(e.f32);
            else if(type == 'd') writer.write(e.f64);
            else if(type == 'l') writer.write((int64_t)(e.i64));
            else if(type == 'i') writer.write(e.i32);
            else if(type == 'b') writer.write((uint8_t)(e.boolean ? 1 : 0));
            else throw std::string("Invalid property");
        }
    }
}

// primitive values
FBXProperty::FBXProperty(int16_t a) { type = 'Y'; value.i16 = a; }
FBXProperty::FBXProperty(bool a) { type = 'C'; value.boolean = a; }
FBXProperty::FBXProperty(int32_t a) { type = 'I'; value.i32 = a; }
FBXProperty::FBXProperty(float a) { type = 'F'; value.f32 = a; }
FBXProperty::FBXProperty(double a) { type = 'D'; value.f64 = a; }
FBXProperty::FBXProperty(int64_t a) { type = 'L'; value.i64 = a; }
// arrays
FBXProperty::FBXProperty(const std::vector<bool> a) : type('b') {
    for(auto el : a) {
        FBXPropertyValue v;
        v.boolean = el;
        values.push_back(v);
    }
}
FBXProperty::FBXProperty(const std::vector<int32_t> a) : type('i') {
    for(auto el : a) {
        FBXPropertyValue v;
        v.i32 = el;
        values.push_back(v);
    }
}
FBXProperty::FBXProperty(const std::vector<float> a) : type('f') {
    for(auto el : a) {
        FBXPropertyValue v;
        v.f32 = el;
        values.push_back(v);
    }
}
FBXProperty::FBXProperty(const std::vector<double> a) : type('d') {
    for(auto el : a) {
        FBXPropertyValue v;
        v.f64 = el;
        values.push_back(v);
    }
}
FBXProperty::FBXProperty(const std::vector<int64_t> a) : type('l') {
    for(auto el : a) {
        FBXPropertyValue v;
        v.i64 = el;
        values.push_back(v);
    }
}
// raw / string
FBXProperty::FBXProperty(const std::vector<uint8_t> a, uint8_t type): raw(a) {
    if(type != 'R' && type != 'S') {
        throw std::string("Bad argument to FBXProperty constructor");
    }
    this->type = type;
}
// string
FBXProperty::FBXProperty(const std::string a){
    for(uint8_t v : a) {
        raw.push_back(v);
    }
    this->type = 'S';
}
FBXProperty::FBXProperty(const char *a){
    for(;*a != 0; a++) {
        raw.push_back(*a);
    }
    this->type = 'S';
}

namespace {
    char base16Letter(uint8_t n) {
        n %= 16;
        if(n <= 9) return n + '0';
        return n + 'a' - 10;
    }
    std::string base16Number(uint8_t n) {
        return std::string() + base16Letter(n >> 4) + base16Letter(n);
    }
}

char FBXProperty::getType()
{
    return type;
}

string FBXProperty::to_string()
{
    if(type == 'Y') return std::to_string(value.i16);
    else if(type == 'C') return value.boolean ? "true" : "false";
    else if(type == 'I') return std::to_string(value.i32);
    else if(type == 'F') return std::to_string(value.f32);
    else if(type == 'D') return std::to_string(value.f64);
    else if(type == 'L') return std::to_string(value.i64);
    else if(type == 'R') {
        string s("\"");
        for(char c : raw) {
            s += std::to_string(c) + " ";
        }
        return s + "\"";
    } else if(type == 'S') {
        string s("\"");
        for(uint8_t c : raw) {
            if(c == '\\') s += "\\\\";
            else if(c >= 32 && c <= 126) s += c;
            else s = s + "\\u00" + base16Number(c);
        }
        return s + "\"";
    } else {
        string s("[");
        bool hasPrev = false;
        for(auto e : values) {
            if(hasPrev) s += ", ";
            if(type == 'f') s += std::to_string(e.f32);
            else if(type == 'd') s += std::to_string(e.f64);
            else if(type == 'l') s += std::to_string(e.i64);
            else if(type == 'i') s += std::to_string(e.i32);
            else if(type == 'b') s += (e.boolean ? "true" : "false");
            hasPrev = true;
        }
        return s+"]";
    }
    throw std::string("Invalid property");
}

uint32_t FBXProperty::getBytes()
{
    if(type == 'Y') return 2 + 1; // 2 for int16, 1 for type spec
    else if(type == 'C') return 1 + 1;
    else if(type == 'I') return 4 + 1;
    else if(type == 'F') return 4 + 1;
    else if(type == 'D') return 8 + 1;
    else if(type == 'L') return 8 + 1;
    else if(type == 'R') return raw.size() + 5;
    else if(type == 'S') return raw.size() + 5;
    else if(type == 'f') return values.size() * 4 + 13;
    else if(type == 'd') return values.size() * 8 + 13;
    else if(type == 'l') return values.size() * 8 + 13;
    else if(type == 'i') return values.size() * 4 + 13;
    else if(type == 'b') return values.size() * 1 + 13;
    throw std::string("Invalid property");
}

} // namespace fbx
