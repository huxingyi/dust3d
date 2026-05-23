#include "glb_reader.h"
#include "json.hpp"
#include <QDebug>
#include <cstring>

using json = nlohmann::json;

// Helper to safely read a typed value from an unaligned buffer via memcpy
template <typename T>
static inline T readUnaligned(const uint8_t* ptr)
{
    T value;
    memcpy(&value, ptr, sizeof(T));
    return value;
}

// Compute byte stride per element for a given glTF type string
static size_t componentCount(const std::string& type)
{
    if (type == "SCALAR")
        return 1;
    if (type == "VEC2")
        return 2;
    if (type == "VEC3")
        return 3;
    if (type == "VEC4")
        return 4;
    if (type == "MAT2")
        return 4;
    if (type == "MAT3")
        return 9;
    if (type == "MAT4")
        return 16;
    return 0;
}

static size_t componentByteSize(int componentType)
{
    switch (componentType) {
    case 5120:
        return 1; // BYTE
    case 5121:
        return 1; // UNSIGNED_BYTE
    case 5122:
        return 2; // SHORT
    case 5123:
        return 2; // UNSIGNED_SHORT
    case 5125:
        return 4; // UNSIGNED_INT
    case 5126:
        return 4; // FLOAT
    default:
        return 0;
    }
}

bool GlbReader::read(const QByteArray& data, dust3d::MeshGenerator::ImportedModelData& result, QImage* textureImage)
{
    if (data.size() < 12)
        return false;

    const uint8_t* ptr = (const uint8_t*)data.constData();
    const size_t dataSize = (size_t)data.size();

    uint32_t magic;
    memcpy(&magic, ptr, 4);
    if (magic != 0x46546C67) // "glTF"
        return false;

    uint32_t version;
    memcpy(&version, ptr + 4, 4);
    if (version != 2)
        return false;

    uint32_t totalLength;
    memcpy(&totalLength, ptr + 8, 4);

    if ((size_t)totalLength > dataSize)
        return false;

    // Parse chunks
    const uint8_t* jsonChunkData = nullptr;
    uint32_t jsonChunkLength = 0;
    const uint8_t* binChunkData = nullptr;
    uint32_t binChunkLength = 0;

    size_t offset = 12;
    while (offset + 8 <= totalLength) {
        uint32_t chunkLength;
        memcpy(&chunkLength, ptr + offset, 4);
        uint32_t chunkType;
        memcpy(&chunkType, ptr + offset + 4, 4);

        // Validate chunk data fits within the file
        size_t chunkDataEnd = offset + 8 + (size_t)chunkLength;
        if (chunkDataEnd < offset + 8 || chunkDataEnd > totalLength)
            return false; // Overflow or out-of-bounds chunk

        if (chunkType == 0x4E4F534A) { // "JSON"
            jsonChunkData = ptr + offset + 8;
            jsonChunkLength = chunkLength;
        } else if (chunkType == 0x004E4942) { // "BIN\0"
            binChunkData = ptr + offset + 8;
            binChunkLength = chunkLength;
        }
        offset = chunkDataEnd;
    }

    if (!jsonChunkData || !binChunkData)
        return false;

    json gltf;
    try {
        gltf = json::parse(jsonChunkData, jsonChunkData + jsonChunkLength);
    } catch (...) {
        return false;
    }

    if (!gltf.count("accessors") || !gltf.count("bufferViews"))
        return false;

    if (!gltf.count("meshes") || gltf["meshes"].empty())
        return false;

    // Safe accessor reader: validates array indices, type/componentType match, and full byte range
    auto getAccessor = [&](int accessorIndex, int expectedComponentType, const std::string& expectedType) -> std::pair<const uint8_t*, size_t> {
        if (accessorIndex < 0 || accessorIndex >= (int)gltf["accessors"].size())
            return { nullptr, 0 };

        const auto& accessor = gltf["accessors"][accessorIndex];
        if (!accessor.count("bufferView") || !accessor.count("count") || !accessor.count("componentType") || !accessor.count("type"))
            return { nullptr, 0 };

        int bufferViewIndex = accessor["bufferView"];
        if (bufferViewIndex < 0 || bufferViewIndex >= (int)gltf["bufferViews"].size())
            return { nullptr, 0 };

        size_t count = accessor["count"];
        int compType = accessor["componentType"];
        std::string type = accessor["type"];

        if (compType != expectedComponentType || type != expectedType)
            return { nullptr, 0 };

        size_t numComponents = componentCount(type);
        size_t compSize = componentByteSize(compType);
        if (numComponents == 0 || compSize == 0)
            return { nullptr, 0 };

        size_t elementSize = numComponents * compSize;

        const auto& bufferView = gltf["bufferViews"][bufferViewIndex];
        size_t byteOffset = bufferView.value("byteOffset", 0);
        size_t accessorByteOffset = accessor.value("byteOffset", 0);

        size_t startOffset = byteOffset + accessorByteOffset;
        if (startOffset < byteOffset) // overflow check
            return { nullptr, 0 };

        // Validate the full data range fits within the BIN chunk
        size_t totalBytes = count * elementSize;
        if (count > 0 && totalBytes / count != elementSize) // overflow check
            return { nullptr, 0 };
        if (startOffset + totalBytes < startOffset) // overflow check
            return { nullptr, 0 };
        if (startOffset + totalBytes > binChunkLength)
            return { nullptr, 0 };

        return { binChunkData + startOffset, count };
    };

    const auto& mesh = gltf["meshes"][0];
    if (!mesh.count("primitives") || mesh["primitives"].empty())
        return false;

    for (const auto& primitive : mesh["primitives"]) {
        if (!primitive.count("attributes"))
            continue;

        const auto& attributes = primitive["attributes"];

        // Read positions
        if (!attributes.count("POSITION"))
            continue;
        auto [posData, posCount] = getAccessor(attributes["POSITION"], 5126, "VEC3"); // FLOAT, VEC3
        if (!posData || posCount == 0)
            continue;

        size_t baseVertex = result.vertices.size();
        for (size_t i = 0; i < posCount; ++i) {
            float x = readUnaligned<float>(posData + i * 3 * sizeof(float));
            float y = readUnaligned<float>(posData + i * 3 * sizeof(float) + sizeof(float));
            float z = readUnaligned<float>(posData + i * 3 * sizeof(float) + 2 * sizeof(float));
            result.vertices.emplace_back((double)x, (double)y, (double)z);
        }

        // Read normals
        if (attributes.count("NORMAL")) {
            auto [normData, normCount] = getAccessor(attributes["NORMAL"], 5126, "VEC3");
            if (normData && normCount == posCount) {
                size_t prevSize = result.vertexNormals.size();
                result.vertexNormals.resize(prevSize + posCount);
                for (size_t i = 0; i < posCount; ++i) {
                    float nx = readUnaligned<float>(normData + i * 3 * sizeof(float));
                    float ny = readUnaligned<float>(normData + i * 3 * sizeof(float) + sizeof(float));
                    float nz = readUnaligned<float>(normData + i * 3 * sizeof(float) + 2 * sizeof(float));
                    result.vertexNormals[prevSize + i] = dust3d::Vector3((double)nx, (double)ny, (double)nz);
                }
            }
        }

        // Read vertex colors
        if (attributes.count("COLOR_0")) {
            int colorAccessorIndex = (int)attributes["COLOR_0"];
            if (colorAccessorIndex >= 0 && colorAccessorIndex < (int)gltf["accessors"].size()) {
                const auto& colorAccessor = gltf["accessors"][colorAccessorIndex];
                int colorComponentType = colorAccessor.value("componentType", 0);
                std::string colorType = colorAccessor.value("type", "");
                size_t colorCount = colorAccessor.value("count", (size_t)0);

                if (colorAccessor.count("bufferView") && colorCount == posCount) {
                    int colorBufView = colorAccessor["bufferView"];
                    if (colorBufView >= 0 && colorBufView < (int)gltf["bufferViews"].size()) {
                        const auto& colorBufViewObj = gltf["bufferViews"][colorBufView];
                        size_t colorByteOffset = colorBufViewObj.value("byteOffset", 0) + colorAccessor.value("byteOffset", 0);

                        int stride = (colorType == "VEC4") ? 4 : 3;
                        size_t colorCompSize = componentByteSize(colorComponentType);
                        size_t colorElementSize = stride * colorCompSize;
                        size_t colorTotalBytes = posCount * colorElementSize;

                        // Validate full color data range
                        if (colorCompSize > 0
                            && colorByteOffset >= (size_t)colorBufViewObj.value("byteOffset", 0) // no overflow
                            && (posCount == 0 || colorTotalBytes / posCount == colorElementSize) // no overflow
                            && colorByteOffset + colorTotalBytes >= colorByteOffset // no overflow
                            && colorByteOffset + colorTotalBytes <= binChunkLength) {

                            const uint8_t* colorData = binChunkData + colorByteOffset;

                            size_t prevSize = result.vertexColors.size();
                            result.vertexColors.resize(prevSize + posCount, dust3d::Color(1.0, 1.0, 1.0));

                            if (colorComponentType == 5126 && (colorType == "VEC3" || colorType == "VEC4")) {
                                for (size_t i = 0; i < posCount; ++i) {
                                    float cr = readUnaligned<float>(colorData + i * stride * sizeof(float));
                                    float cg = readUnaligned<float>(colorData + i * stride * sizeof(float) + sizeof(float));
                                    float cb = readUnaligned<float>(colorData + i * stride * sizeof(float) + 2 * sizeof(float));
                                    result.vertexColors[prevSize + i] = dust3d::Color(cr, cg, cb);
                                }
                            } else if (colorComponentType == 5121 && (colorType == "VEC3" || colorType == "VEC4")) {
                                for (size_t i = 0; i < posCount; ++i) {
                                    result.vertexColors[prevSize + i] = dust3d::Color(
                                        colorData[i * stride] / 255.0,
                                        colorData[i * stride + 1] / 255.0,
                                        colorData[i * stride + 2] / 255.0);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Read UVs
        std::vector<dust3d::Vector2> uvs;
        if (attributes.count("TEXCOORD_0")) {
            auto [uvData, uvCount] = getAccessor(attributes["TEXCOORD_0"], 5126, "VEC2");
            if (uvData && uvCount == posCount) {
                uvs.resize(uvCount);
                for (size_t i = 0; i < uvCount; ++i) {
                    float u = readUnaligned<float>(uvData + i * 2 * sizeof(float));
                    float v = readUnaligned<float>(uvData + i * 2 * sizeof(float) + sizeof(float));
                    uvs[i] = dust3d::Vector2(u, v);
                }
            }
        }

        // Read indices
        if (primitive.count("indices")) {
            int indicesAccessorIndex = primitive["indices"];
            if (indicesAccessorIndex >= 0 && indicesAccessorIndex < (int)gltf["accessors"].size()) {
                const auto& accessor = gltf["accessors"][indicesAccessorIndex];
                int compType = accessor.value("componentType", 0);
                size_t count = accessor.value("count", (size_t)0);

                if (accessor.count("bufferView")) {
                    int bufferViewIndex = accessor["bufferView"];
                    if (bufferViewIndex >= 0 && bufferViewIndex < (int)gltf["bufferViews"].size()) {
                        const auto& bufferView = gltf["bufferViews"][bufferViewIndex];
                        size_t byteOffset = bufferView.value("byteOffset", 0);
                        size_t accessorByteOffset = accessor.value("byteOffset", 0);
                        size_t startOffset = byteOffset + accessorByteOffset;

                        // Determine element size and validate full range
                        size_t indexSize = componentByteSize(compType);
                        if (indexSize > 0 && startOffset >= byteOffset) {
                            size_t totalBytes = count * indexSize;
                            if (count == 0 || totalBytes / count == indexSize) {
                                if (startOffset + totalBytes >= startOffset && startOffset + totalBytes <= binChunkLength) {
                                    const uint8_t* indexData = binChunkData + startOffset;

                                    for (size_t i = 0; i + 2 < count; i += 3) {
                                        size_t i0, i1, i2;
                                        if (compType == 5123) { // UNSIGNED_SHORT
                                            i0 = readUnaligned<uint16_t>(indexData + i * sizeof(uint16_t));
                                            i1 = readUnaligned<uint16_t>(indexData + (i + 1) * sizeof(uint16_t));
                                            i2 = readUnaligned<uint16_t>(indexData + (i + 2) * sizeof(uint16_t));
                                        } else if (compType == 5125) { // UNSIGNED_INT
                                            i0 = readUnaligned<uint32_t>(indexData + i * sizeof(uint32_t));
                                            i1 = readUnaligned<uint32_t>(indexData + (i + 1) * sizeof(uint32_t));
                                            i2 = readUnaligned<uint32_t>(indexData + (i + 2) * sizeof(uint32_t));
                                        } else {
                                            continue;
                                        }

                                        // Validate indices are within vertex bounds
                                        if (i0 >= posCount || i1 >= posCount || i2 >= posCount)
                                            continue;

                                        result.faces.push_back({ baseVertex + i0, baseVertex + i1, baseVertex + i2 });

                                        if (!uvs.empty()) {
                                            dust3d::PositionKey pk0(result.vertices[baseVertex + i0]);
                                            dust3d::PositionKey pk1(result.vertices[baseVertex + i1]);
                                            dust3d::PositionKey pk2(result.vertices[baseVertex + i2]);
                                            result.triangleUvs[{ pk0, pk1, pk2 }] = { uvs[i0], uvs[i1], uvs[i2] };
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else {
            // Non-indexed geometry
            for (size_t i = 0; i + 2 < posCount; i += 3) {
                result.faces.push_back({ baseVertex + i, baseVertex + i + 1, baseVertex + i + 2 });
                if (!uvs.empty()) {
                    dust3d::PositionKey pk0(result.vertices[baseVertex + i]);
                    dust3d::PositionKey pk1(result.vertices[baseVertex + i + 1]);
                    dust3d::PositionKey pk2(result.vertices[baseVertex + i + 2]);
                    result.triangleUvs[{ pk0, pk1, pk2 }] = { uvs[i], uvs[i + 1], uvs[i + 2] };
                }
            }
        }
    }

    // Extract texture image or base color from the first material
    if (textureImage && gltf.count("materials") && !gltf["materials"].empty()) {
        const auto& material = gltf["materials"][0];
        if (material.count("pbrMetallicRoughness")) {
            const auto& pbr = material["pbrMetallicRoughness"];
            if (pbr.count("baseColorTexture")) {
                int textureIndex = pbr["baseColorTexture"]["index"];
                if (gltf.count("textures") && textureIndex >= 0 && textureIndex < (int)gltf["textures"].size()) {
                    const auto& texture = gltf["textures"][textureIndex];
                    if (texture.count("source")) {
                        int imageIndex = texture["source"];
                        if (gltf.count("images") && imageIndex >= 0 && imageIndex < (int)gltf["images"].size()) {
                            const auto& image = gltf["images"][imageIndex];
                            if (image.count("bufferView")) {
                                int imgBufView = image["bufferView"];
                                if (imgBufView >= 0 && imgBufView < (int)gltf["bufferViews"].size()) {
                                    const auto& bufView = gltf["bufferViews"][imgBufView];
                                    size_t imgOffset = bufView.value("byteOffset", 0);
                                    size_t imgLength = bufView.value("byteLength", (size_t)0);
                                    if (imgLength > 0
                                        && imgOffset + imgLength >= imgOffset // overflow check
                                        && imgOffset + imgLength <= binChunkLength) {
                                        *textureImage = QImage::fromData(binChunkData + imgOffset, (int)imgLength);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (textureImage->isNull() && pbr.count("baseColorFactor")) {
                const auto& factor = pbr["baseColorFactor"];
                if (factor.size() >= 3) {
                    int r = (int)(factor[0].get<float>() * 255);
                    int g = (int)(factor[1].get<float>() * 255);
                    int b = (int)(factor[2].get<float>() * 255);
                    int a = factor.size() >= 4 ? (int)(factor[3].get<float>() * 255) : 255;
                    *textureImage = QImage(2, 2, QImage::Format_ARGB32);
                    textureImage->fill(QColor(r, g, b, a));
                }
            }
        }
    }

    return !result.vertices.empty() && !result.faces.empty();
}
