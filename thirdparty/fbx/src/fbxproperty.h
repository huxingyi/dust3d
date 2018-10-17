#ifndef FBXPROPERTY_H
#define FBXPROPERTY_H

#include <memory>
#include <iostream>
#include <vector>

namespace fbx {

// WARNING: (copied from fbxutil.h)
// this assumes that float is 32bit and double is 64bit
// both conforming to IEEE 754, it does not assume endianness
// it also assumes that signed integers are two's complement
union FBXPropertyValue {
    int16_t i16;  // Y
    bool boolean; // C, b
    int32_t i32;  // I, i
    float f32;    // F, f
    double f64;   // D, d
    int64_t i64;  // L, l
};

class FBXProperty
{
public:
    FBXProperty(std::ifstream &input);
    // primitive values
    FBXProperty(int16_t);
    FBXProperty(bool);
    FBXProperty(int32_t);
    FBXProperty(float);
    FBXProperty(double);
    FBXProperty(int64_t);
    // arrays
    FBXProperty(const std::vector<bool>);
    FBXProperty(const std::vector<int32_t>);
    FBXProperty(const std::vector<float>);
    FBXProperty(const std::vector<double>);
    FBXProperty(const std::vector<int64_t>);
    // raw / string
    FBXProperty(const std::vector<uint8_t>, uint8_t type);
    FBXProperty(const std::string);
    FBXProperty(const char *);

    void write(std::ofstream &output);

    std::string to_string();
    char getType();

    bool is_array();
    uint32_t getBytes();
private:
    uint8_t type;
    FBXPropertyValue value;
    std::vector<uint8_t> raw;
    std::vector<FBXPropertyValue> values;
};

} // namespace fbx

#endif // FBXPROPERTY_H
