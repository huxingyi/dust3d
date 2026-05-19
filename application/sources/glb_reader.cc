#include "glb_reader.h"
#include "json.hpp"
#include <QDebug>
#include <cstring>

using json = nlohmann::json;

bool GlbReader::read(const QByteArray& data, dust3d::MeshGenerator::ImportedModelData& result, QImage* textureImage)
{
    if (data.size() < 12)
        return false;

    const uint8_t* ptr = (const uint8_t*)data.constData();

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

    if ((int)totalLength > data.size())
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

        if (chunkType == 0x4E4F534A) { // "JSON"
            jsonChunkData = ptr + offset + 8;
            jsonChunkLength = chunkLength;
        } else if (chunkType == 0x004E4942) { // "BIN\0"
            binChunkData = ptr + offset + 8;
            binChunkLength = chunkLength;
        }
        offset += 8 + chunkLength;
    }

    if (!jsonChunkData || !binChunkData)
        return false;

    json gltf;
    try {
        gltf = json::parse(jsonChunkData, jsonChunkData + jsonChunkLength);
    } catch (...) {
        return false;
    }

    if (!gltf.count("meshes") || gltf["meshes"].empty())
        return false;

    auto getAccessor = [&](int accessorIndex, int expectedComponentType, const std::string& expectedType) -> std::pair<const uint8_t*, size_t> {
        const auto& accessor = gltf["accessors"][accessorIndex];
        int bufferViewIndex = accessor["bufferView"];
        size_t count = accessor["count"];
        int componentType = accessor["componentType"];
        std::string type = accessor["type"];

        if (componentType != expectedComponentType || type != expectedType)
            return { nullptr, 0 };

        const auto& bufferView = gltf["bufferViews"][bufferViewIndex];
        size_t byteOffset = bufferView.value("byteOffset", 0);
        size_t accessorByteOffset = accessor.value("byteOffset", 0);

        if (byteOffset + accessorByteOffset >= binChunkLength)
            return { nullptr, 0 };

        return { binChunkData + byteOffset + accessorByteOffset, count };
    };

    const auto& mesh = gltf["meshes"][0];
    if (!mesh.count("primitives") || mesh["primitives"].empty())
        return false;

    size_t vertexOffset = 0;

    for (const auto& primitive : mesh["primitives"]) {
        if (!primitive.count("attributes"))
            continue;

        const auto& attributes = primitive["attributes"];

        // Read positions
        if (!attributes.count("POSITION"))
            continue;
        auto [posData, posCount] = getAccessor(attributes["POSITION"], 5126, "VEC3"); // FLOAT, VEC3
        if (!posData)
            continue;

        size_t baseVertex = result.vertices.size();
        const float* positions = (const float*)posData;
        for (size_t i = 0; i < posCount; ++i) {
            result.vertices.emplace_back(
                (double)positions[i * 3 + 0],
                (double)positions[i * 3 + 1],
                (double)positions[i * 3 + 2]);
        }

        // Read normals
        if (attributes.count("NORMAL")) {
            auto [normData, normCount] = getAccessor(attributes["NORMAL"], 5126, "VEC3");
            if (normData && normCount == posCount) {
                size_t prevSize = result.vertexNormals.size();
                result.vertexNormals.resize(prevSize + posCount);
                const float* normals = (const float*)normData;
                for (size_t i = 0; i < posCount; ++i) {
                    result.vertexNormals[prevSize + i] = dust3d::Vector3(
                        (double)normals[i * 3 + 0],
                        (double)normals[i * 3 + 1],
                        (double)normals[i * 3 + 2]);
                }
            }
        }

        // Read vertex colors
        if (attributes.count("COLOR_0")) {
            const auto& colorAccessor = gltf["accessors"][(int)attributes["COLOR_0"]];
            int colorComponentType = colorAccessor["componentType"];
            std::string colorType = colorAccessor["type"];
            int colorBufView = colorAccessor["bufferView"];
            size_t colorCount = colorAccessor["count"];
            const auto& colorBufViewObj = gltf["bufferViews"][colorBufView];
            size_t colorByteOffset = colorBufViewObj.value("byteOffset", 0) + colorAccessor.value("byteOffset", 0);
            const uint8_t* colorData = binChunkData + colorByteOffset;

            if (colorCount == posCount) {
                size_t prevSize = result.vertexColors.size();
                result.vertexColors.resize(prevSize + posCount, dust3d::Color(1.0, 1.0, 1.0));
                if (colorComponentType == 5126 && (colorType == "VEC3" || colorType == "VEC4")) {
                    const float* cf = (const float*)colorData;
                    int stride = (colorType == "VEC4") ? 4 : 3;
                    for (size_t i = 0; i < posCount; ++i) {
                        result.vertexColors[prevSize + i] = dust3d::Color(cf[i * stride], cf[i * stride + 1], cf[i * stride + 2]);
                    }
                } else if (colorComponentType == 5121 && (colorType == "VEC3" || colorType == "VEC4")) {
                    int stride = (colorType == "VEC4") ? 4 : 3;
                    for (size_t i = 0; i < posCount; ++i) {
                        result.vertexColors[prevSize + i] = dust3d::Color(
                            colorData[i * stride] / 255.0,
                            colorData[i * stride + 1] / 255.0,
                            colorData[i * stride + 2] / 255.0);
                    }
                }
            }
        }

        // Read UVs
        std::vector<dust3d::Vector2> uvs;
        if (attributes.count("TEXCOORD_0")) {
            auto [uvData, uvCount] = getAccessor(attributes["TEXCOORD_0"], 5126, "VEC2");
            if (uvData && uvCount == posCount) {
                const float* texcoords = (const float*)uvData;
                uvs.resize(uvCount);
                for (size_t i = 0; i < uvCount; ++i) {
                    uvs[i] = dust3d::Vector2(texcoords[i * 2 + 0], texcoords[i * 2 + 1]);
                }
            }
        }

        // Read indices
        if (primitive.count("indices")) {
            int indicesAccessor = primitive["indices"];
            const auto& accessor = gltf["accessors"][indicesAccessor];
            int componentType = accessor["componentType"];
            int bufferViewIndex = accessor["bufferView"];
            size_t count = accessor["count"];
            const auto& bufferView = gltf["bufferViews"][bufferViewIndex];
            size_t byteOffset = bufferView.value("byteOffset", 0);
            size_t accessorByteOffset = accessor.value("byteOffset", 0);
            const uint8_t* indexData = binChunkData + byteOffset + accessorByteOffset;

            for (size_t i = 0; i + 2 < count; i += 3) {
                size_t i0, i1, i2;
                if (componentType == 5123) { // UNSIGNED_SHORT
                    const uint16_t* indices = (const uint16_t*)indexData;
                    i0 = indices[i];
                    i1 = indices[i + 1];
                    i2 = indices[i + 2];
                } else if (componentType == 5125) { // UNSIGNED_INT
                    const uint32_t* indices = (const uint32_t*)indexData;
                    i0 = indices[i];
                    i1 = indices[i + 1];
                    i2 = indices[i + 2];
                } else {
                    continue;
                }

                result.faces.push_back({ baseVertex + i0, baseVertex + i1, baseVertex + i2 });

                if (!uvs.empty()) {
                    dust3d::PositionKey pk0(result.vertices[baseVertex + i0]);
                    dust3d::PositionKey pk1(result.vertices[baseVertex + i1]);
                    dust3d::PositionKey pk2(result.vertices[baseVertex + i2]);
                    result.triangleUvs[{ pk0, pk1, pk2 }] = { uvs[i0], uvs[i1], uvs[i2] };
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

        vertexOffset += posCount;
    }

    // Extract texture image or base color from the first material
    if (textureImage && gltf.count("materials") && !gltf["materials"].empty()) {
        const auto& material = gltf["materials"][0];
        if (material.count("pbrMetallicRoughness")) {
            const auto& pbr = material["pbrMetallicRoughness"];
            if (pbr.count("baseColorTexture")) {
                int textureIndex = pbr["baseColorTexture"]["index"];
                if (gltf.count("textures") && textureIndex < (int)gltf["textures"].size()) {
                    const auto& texture = gltf["textures"][textureIndex];
                    if (texture.count("source")) {
                        int imageIndex = texture["source"];
                        if (gltf.count("images") && imageIndex < (int)gltf["images"].size()) {
                            const auto& image = gltf["images"][imageIndex];
                            if (image.count("bufferView")) {
                                int imgBufView = image["bufferView"];
                                const auto& bufView = gltf["bufferViews"][imgBufView];
                                size_t imgOffset = bufView.value("byteOffset", 0);
                                size_t imgLength = bufView["byteLength"];
                                if (imgOffset + imgLength <= binChunkLength) {
                                    *textureImage = QImage::fromData(binChunkData + imgOffset, (int)imgLength);
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
