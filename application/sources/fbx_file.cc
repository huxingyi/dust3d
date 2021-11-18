#include <fbxnode.h>
#include <fbxproperty.h>
#include <QDateTime>
#include <QtMath>
#include <QtCore/qbuffer.h>
#include <QByteArray>
#include <QFileInfo>
#include "fbx_file.h"
#include "version.h"
#include "document.h"

using namespace fbx;

std::vector<double> FbxFileWriter::m_identityMatrix = {
    1.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 1.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 1.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 1.000000
};

void FbxFileWriter::createFbxHeader()
{
    FBXNode headerExtension("FBXHeaderExtension");
    headerExtension.addPropertyNode("FBXHeaderVersion", (int32_t)1003);
    headerExtension.addPropertyNode("FBXVersion", (int32_t)m_fbxDocument.getVersion());
    //headerExtension.addPropertyNode("FBXVersion", (int32_t)7500);
    headerExtension.addPropertyNode("EncryptionType", (int32_t)0);
    {
        auto currentDateTime = QDateTime::currentDateTime();
        const auto &currentDate = currentDateTime.date();
        const auto &currentTime = currentDateTime.time();
        
        FBXNode creationTimeStamp("CreationTimeStamp");
        creationTimeStamp.addPropertyNode("Version", (int32_t)1000);
        creationTimeStamp.addPropertyNode("Year", (int32_t)currentDate.year());
        creationTimeStamp.addPropertyNode("Month", (int32_t)currentDate.month());
        creationTimeStamp.addPropertyNode("Day", (int32_t)currentDate.day());
        creationTimeStamp.addPropertyNode("Hour", (int32_t)currentTime.hour());
        creationTimeStamp.addPropertyNode("Minute", (int32_t)currentTime.minute());
        creationTimeStamp.addPropertyNode("Second", (int32_t)currentTime.second());
        creationTimeStamp.addPropertyNode("Millisecond", (int32_t)0);
        creationTimeStamp.addChild(FBXNode());
        headerExtension.addChild(creationTimeStamp);
    }
    headerExtension.addPropertyNode("Creator", APP_NAME " " APP_HUMAN_VER);
    {
        FBXNode sceneInfo("SceneInfo");
        sceneInfo.addProperty(std::vector<uint8_t>({'G','l','o','b','a','l','I','n','f','o',0,1,'S','c','e','n','e','I','n','f','o'}), 'S');
        sceneInfo.addProperty("UserData");
        sceneInfo.addPropertyNode("Type", "UserData");
        sceneInfo.addPropertyNode("Version", 100);
        {
            FBXNode metadata("MetaData");
            metadata.addPropertyNode("Version", 100);
            metadata.addPropertyNode("Title", "");
            metadata.addPropertyNode("Subject", "");
            metadata.addPropertyNode("Author", "");
            metadata.addPropertyNode("Keywords", "");
            metadata.addPropertyNode("Revision", "");
            metadata.addPropertyNode("Comment", "");
            metadata.addChild(FBXNode());
            sceneInfo.addChild(metadata);
        }
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("DocumentUrl");
                p.addProperty("KString");
                p.addProperty("Url");
                p.addProperty("");
                p.addProperty("/foobar.fbx");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("SrcDocumentUrl");
                p.addProperty("KString");
                p.addProperty("Url");
                p.addProperty("");
                p.addProperty("/foobar.fbx");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original");
                p.addProperty("Compound");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original|ApplicationVendor");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty(APP_COMPANY);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original|ApplicationName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty(APP_NAME);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original|ApplicationVersion");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty(APP_HUMAN_VER);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original|DateTime_GMT");
                p.addProperty("DateTime");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("01/01/1970 00:00:00.000");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Original|FileName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("/foobar.fbx");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastSaved");
                p.addProperty("Compound");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastSaved|ApplicationVendor");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty(APP_COMPANY);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastSaved|ApplicationName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty(APP_NAME);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastSaved|ApplicationVersion");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty(APP_HUMAN_VER);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastSaved|DateTime_GMT");
                p.addProperty("DateTime");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("01/01/1970 00:00:00.000");
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            sceneInfo.addChild(properties);
            sceneInfo.addChild(FBXNode());
        }
        headerExtension.addChild(sceneInfo);
        headerExtension.addChild(FBXNode());
    }
    
    m_fbxDocument.nodes.push_back(headerExtension);
}

void FbxFileWriter::createCreationTime()
{
    FBXNode creationTime("CreationTime");
    creationTime.addProperty("1970-01-01 10:00:00:000");
    m_fbxDocument.nodes.push_back(creationTime);
}

void FbxFileWriter::createFileId()
{
    std::vector<uint8_t> fileIdBytes = {40, (uint8_t)-77, 42, (uint8_t)-21, (uint8_t)-74, 36, (uint8_t)-52, (uint8_t)-62, (uint8_t)-65, (uint8_t)-56, (uint8_t)-80, 42, (uint8_t)-87, 43, (uint8_t)-4, (uint8_t)-15};
    FBXNode fileId("FileId");
    fileId.addProperty(fileIdBytes, 'R');
    m_fbxDocument.nodes.push_back(fileId);
}

void FbxFileWriter::createCreator()
{
    FBXNode creator("Creator");
    creator.addProperty(APP_NAME " " APP_HUMAN_VER);
    m_fbxDocument.nodes.push_back(creator);
}

void FbxFileWriter::createGlobalSettings()
{
    FBXNode globalSettings("GlobalSettings");
    globalSettings.addPropertyNode("Version", (int32_t)1000);
    {
        FBXNode properties("Properties70");
        {
            FBXNode p("P");
            p.addProperty("UpAxis");
            p.addProperty("int");
            p.addProperty("Integer");
            p.addProperty("");
            p.addProperty((int32_t)1);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("UpAxisSign");
            p.addProperty("int");
            p.addProperty("Integer");
            p.addProperty("");
            p.addProperty((int32_t)1);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("FrontAxis");
            p.addProperty("int");
            p.addProperty("Integer");
            p.addProperty("");
            p.addProperty((int32_t)2);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("FrontAxisSign");
            p.addProperty("int");
            p.addProperty("Integer");
            p.addProperty("");
            p.addProperty((int32_t)1);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("CoordAxis");
            p.addProperty("int");
            p.addProperty("Integer");
            p.addProperty("");
            p.addProperty((int32_t)0);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("CoordAxisSign");
            p.addProperty("int");
            p.addProperty("Integer");
            p.addProperty("");
            p.addProperty((int32_t)1);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("OriginalUpAxis");
            p.addProperty("int");
            p.addProperty("Integer");
            p.addProperty("");
            p.addProperty((int32_t)-1);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("OriginalUpAxisSign");
            p.addProperty("int");
            p.addProperty("Integer");
            p.addProperty("");
            p.addProperty((int32_t)1);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("UnitScaleFactor");
            p.addProperty("double");
            p.addProperty("Number");
            p.addProperty("");
            p.addProperty((double)100.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("OriginalUnitScaleFactor");
            p.addProperty("double");
            p.addProperty("Number");
            p.addProperty("");
            p.addProperty((double)1.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("AmbientColor");
            p.addProperty("ColorRGB");
            p.addProperty("Color");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("DefaultCamera");
            p.addProperty("KString");
            p.addProperty("");
            p.addProperty("");
            p.addProperty("Producer Perspective");
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("TimeMode");
            p.addProperty("enum");
            p.addProperty("");
            p.addProperty("");
            p.addProperty((int32_t)11);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("TimeSpanStart");
            p.addProperty("KTime");
            p.addProperty("Time");
            p.addProperty("");
            p.addProperty((int64_t)0);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("TimeSpanStop");
            p.addProperty("KTime");
            p.addProperty("Time");
            p.addProperty("");
            p.addProperty((int64_t)46186158000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("CustomFrameRate");
            p.addProperty("double");
            p.addProperty("Number");
            p.addProperty("");
            p.addProperty((double)24.000000);
            properties.addChild(p);
        }
        properties.addChild(FBXNode());
        globalSettings.addChild(properties);
    }
    globalSettings.addChild(FBXNode());
    m_fbxDocument.nodes.push_back(globalSettings);
}

void FbxFileWriter::createDocuments()
{
    FBXNode document("Document");
    document.addProperty((int64_t)m_next64Id++);
    document.addProperty("simple scene");
    document.addProperty("Scene");
    {
        FBXNode properties("Properties70");
        {
            FBXNode p("P");
            p.addProperty("SourceObject");
            p.addProperty("object");
            p.addProperty("");
            p.addProperty("");
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("ActiveAnimStackName");
            p.addProperty("KString");
            p.addProperty("");
            p.addProperty("");
            p.addProperty("");
            properties.addChild(p);
        }
        properties.addChild(FBXNode());
        document.addChild(properties);
    }
    document.addPropertyNode("RootNode", (int64_t)0);
    document.addChild(FBXNode());
    FBXNode documents("Documents");
    documents.addPropertyNode("Count", (int32_t)1);
    documents.addChild(document);
    documents.addChild(FBXNode());
    m_fbxDocument.nodes.push_back(documents);
}

void FbxFileWriter::createReferences()
{
    FBXNode references("References");
    references.addChild(FBXNode());
    m_fbxDocument.nodes.push_back(references);
}

void FbxFileWriter::createDefinitions(size_t deformerCount,
        size_t textureCount,
        size_t videoCount,
        bool hasAnimtion,
        size_t animationStackCount,
        size_t animationLayerCount,
        size_t animationCurveNodeCount,
        size_t animationCurveCount)
{
    FBXNode definitions("Definitions");
    definitions.addPropertyNode("Version", (int32_t)100);
    {
        FBXNode objectType("ObjectType");
        objectType.addProperty("GlobalSettings");
        FBXNode count("Count");
        count.addProperty((int32_t)1);
        objectType.addChild(count);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    {
        FBXNode objectType("ObjectType");
        objectType.addProperty("Geometry");
        FBXNode count("Count");
        count.addProperty((int32_t)1);
        objectType.addChild(count);
        FBXNode propertyTemplate("PropertyTemplate");
        propertyTemplate.addProperty("FbxMesh");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("Color");
                p.addProperty("ColorRGB");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty((double)0.800000);
                p.addProperty((double)0.800000);
                p.addProperty((double)0.800000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("BBoxMin");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("BBoxMax");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Primary Visibility");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)1);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Casts Shadows");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)1);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Receive Shadows");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)1);
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            propertyTemplate.addChild(properties);
        }
        propertyTemplate.addChild(FBXNode());
        objectType.addChild(propertyTemplate);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    {
        FBXNode objectType("ObjectType");
        objectType.addProperty("Model");
        FBXNode count("Count");
        count.addProperty((int32_t)(1 + deformerCount)); // 1 for mesh, deformerCount for limbNodes
        objectType.addChild(count);
        FBXNode propertyTemplate("PropertyTemplate");
        propertyTemplate.addProperty("FbxNode");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("QuaternionInterpolate");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationOffset");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationPivot");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingOffset");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingPivot");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TranslationActive");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TranslationMin");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TranslationMax");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TranslationMinX");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TranslationMinY");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TranslationMinZ");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TranslationMaxX");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TranslationMaxY");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TranslationMaxZ");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationOrder");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationSpaceForLimitOnly");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationStiffnessX");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationStiffnessY");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationStiffnessZ");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("AxisLen");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)10.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("PreRotation");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("PostRotation");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationActive");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationMin");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationMax");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationMinX");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationMinY");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationMinZ");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationMaxX");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationMaxY");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationMaxZ");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("InheritType");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingActive");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingMin");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingMax");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)1.000000);
                p.addProperty((double)1.000000);
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingMinX");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingMinY");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingMinZ");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingMaxX");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingMaxY");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScalingMaxZ");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("GeometricTranslation");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("GeometricRotation");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("GeometricScaling");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)1.000000);
                p.addProperty((double)1.000000);
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MinDampRangeX");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MinDampRangeY");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MinDampRangeZ");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MaxDampRangeX");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MaxDampRangeY");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MaxDampRangeZ");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MinDampStrengthX");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MinDampStrengthY");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MinDampStrengthZ");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MaxDampStrengthX");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MaxDampStrengthY");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MaxDampStrengthZ");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("PreferedAngleX");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("PreferedAngleY");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("PreferedAngleZ");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LookAtProperty");
                p.addProperty("object");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("UpVectorProperty");
                p.addProperty("object");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Show");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)1);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("NegativePercentShapeSupport");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)1);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("DefaultAttributeIndex");
                p.addProperty("int");
                p.addProperty("Integer");
                p.addProperty("");
                p.addProperty((int32_t)-1);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Freeze");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LODBox");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Lcl Translation");
                p.addProperty("Lcl Translation");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Lcl Rotation");
                p.addProperty("Lcl Rotation");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Lcl Scaling");
                p.addProperty("Lcl Scaling");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.000000);
                p.addProperty((double)1.000000);
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Visibility");
                p.addProperty("Visibility");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Visibility Inheritance");
                p.addProperty("Visibility Inheritance");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)1);
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            propertyTemplate.addChild(properties);
        }
        propertyTemplate.addChild(FBXNode());
        objectType.addChild(propertyTemplate);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    {
        FBXNode objectType("ObjectType");
        objectType.addProperty("Implementation");
        objectType.addPropertyNode("Count", (int32_t)1);
        FBXNode propertyTemplate("PropertyTemplate");
        propertyTemplate.addProperty("FbxImplementation");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("ShaderLanguage");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("MentalRaySL");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ShaderLanguageVersion");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RenderAPI");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("MentalRay");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RenderAPIVersion");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RootBindingName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Constants");
                p.addProperty("Compound");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            propertyTemplate.addChild(properties);
        }
        propertyTemplate.addChild(FBXNode());
        objectType.addChild(propertyTemplate);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    {
        FBXNode objectType("ObjectType");
        objectType.addProperty("BindingTable");
        objectType.addPropertyNode("Count", (int32_t)1);
        FBXNode propertyTemplate("PropertyTemplate");
        propertyTemplate.addProperty("FbxBindingTable");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("TargetName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TargetType");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("CodeAbsoluteURL");
                p.addProperty("KString");
                p.addProperty("XRefUrl");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("CodeRelativeURL");
                p.addProperty("KString");
                p.addProperty("XRefUrl");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("CodeTAG");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("shader");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("DescAbsoluteURL");
                p.addProperty("KString");
                p.addProperty("XRefUrl");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("DescRelativeURL");
                p.addProperty("KString");
                p.addProperty("XRefUrl");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("DescTAG");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("shader");
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            propertyTemplate.addChild(properties);
        }
        propertyTemplate.addChild(FBXNode());
        objectType.addChild(propertyTemplate);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    if (deformerCount > 0) {
        FBXNode objectType("ObjectType");
        objectType.addProperty("Deformer");
        objectType.addPropertyNode("Count", (int32_t)deformerCount);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    if (deformerCount > 0) {
        FBXNode objectType("ObjectType");
        objectType.addProperty("NodeAttribute");
        objectType.addPropertyNode("Count", (int32_t)deformerCount);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    if (deformerCount > 0) {
        FBXNode objectType("ObjectType");
        objectType.addProperty("Pose");
        objectType.addPropertyNode("Count", (int32_t)1);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    if (textureCount > 0) {
        FBXNode objectType("ObjectType");
        objectType.addProperty("Texture");
        objectType.addPropertyNode("Count", (int32_t)textureCount);
        FBXNode propertyTemplate("PropertyTemplate");
        propertyTemplate.addProperty("FbxFileTexture");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("TextureTypeUse");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Texture alpha");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("CurrentMappingType");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("WrapModeU");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("WrapModeV");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("UVSwap");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("PremultiplyAlpha");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)1);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Translation");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)0);
                p.addProperty((double)0);
                p.addProperty((double)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Rotation");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)0);
                p.addProperty((double)0);
                p.addProperty((double)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Scaling");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.0);
                p.addProperty((double)1.0);
                p.addProperty((double)1.0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TextureRotationPivot");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.0);
                p.addProperty((double)0.0);
                p.addProperty((double)0.0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TextureScalingPivot");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.0);
                p.addProperty((double)0.0);
                p.addProperty((double)0.0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("CurrentTextureBlendMode");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)1);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("UVSet");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("default");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("UseMaterial");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("UseMipMap");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            propertyTemplate.addChild(properties);
        }
        propertyTemplate.addChild(FBXNode());
        objectType.addChild(propertyTemplate);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    if (videoCount > 0) {
        FBXNode objectType("ObjectType");
        objectType.addProperty("Video");
        objectType.addPropertyNode("Count", (int32_t)videoCount);
        FBXNode propertyTemplate("PropertyTemplate");
        propertyTemplate.addProperty("FbxVideo");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("Path");
                p.addProperty("KString");
                p.addProperty("XRefUrl");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RelPath");
                p.addProperty("KString");
                p.addProperty("XRefUrl");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Color");
                p.addProperty("ColorRGB");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ClipIn");
                p.addProperty("KTime");
                p.addProperty("Time");
                p.addProperty("");
                p.addProperty((int64_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ClipOut");
                p.addProperty("KTime");
                p.addProperty("Time");
                p.addProperty("");
                p.addProperty((int64_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Offset");
                p.addProperty("KTime");
                p.addProperty("Time");
                p.addProperty("");
                p.addProperty((int64_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("PlaySpeed");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("FreeRunning");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Loop");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Mute");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("AccessMode");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ImageSequence");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ImageSequenceOffset");
                p.addProperty("int");
                p.addProperty("Integer");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("FrameRate");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastFrame");
                p.addProperty("int");
                p.addProperty("Integer");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Width");
                p.addProperty("int");
                p.addProperty("Integer");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Height");
                p.addProperty("int");
                p.addProperty("Integer");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("StartFrame");
                p.addProperty("int");
                p.addProperty("Integer");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("StopFrame");
                p.addProperty("int");
                p.addProperty("Integer");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("InterlaceMode");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            propertyTemplate.addChild(properties);
        }
        propertyTemplate.addChild(FBXNode());
        objectType.addChild(propertyTemplate);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    {
        FBXNode objectType("ObjectType");
        objectType.addProperty("Material");
        FBXNode count("Count");
        count.addProperty((int32_t)1);
        objectType.addChild(count);
        FBXNode propertyTemplate("PropertyTemplate");
        propertyTemplate.addProperty("FbxSurfacePhong");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("ShadingModel");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("Phong");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("MultiLayer");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("EmissiveColor");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("EmissiveFactor");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("AmbientColor");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)0.200000);
                p.addProperty((double)0.200000);
                p.addProperty((double)0.200000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("AmbientFactor");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("DiffuseColor");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.000000);
                p.addProperty((double)1.000000);
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("DiffuseFactor");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TransparentColor");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("TransparencyFactor");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Opacity");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("NormalMap");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Bump");
                p.addProperty("Vector3D");
                p.addProperty("Vector");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("BumpFactor");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("DisplacementColor");
                p.addProperty("ColorRGB");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("DisplacementFactor");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("VectorDisplacementColor");
                p.addProperty("ColorRGB");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("VectorDisplacementFactor");
                p.addProperty("double");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("SpecularColor");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)0.200000);
                p.addProperty((double)0.200000);
                p.addProperty((double)0.200000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("SpecularFactor");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Shininess");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)20.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ShininessExponent");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)20.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ReflectionColor");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                p.addProperty((double)0.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ReflectionFactor");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)1.000000);
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            propertyTemplate.addChild(properties);
        }
        propertyTemplate.addChild(FBXNode());
        objectType.addChild(propertyTemplate);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    if (hasAnimtion) {
        FBXNode objectType("ObjectType");
        objectType.addProperty("AnimationStack");
        objectType.addPropertyNode("Count", (int32_t)animationStackCount);
        FBXNode propertyTemplate("PropertyTemplate");
        propertyTemplate.addProperty("FbxAnimStack");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("Description");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LocalStart");
                p.addProperty("KTime");
                p.addProperty("Time");
                p.addProperty("");
                p.addProperty((int64_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LocalStop");
                p.addProperty("KTime");
                p.addProperty("Time");
                p.addProperty("");
                p.addProperty((int64_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ReferenceStart");
                p.addProperty("KTime");
                p.addProperty("Time");
                p.addProperty("");
                p.addProperty((int64_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ReferenceStop");
                p.addProperty("KTime");
                p.addProperty("Time");
                p.addProperty("");
                p.addProperty((int64_t)0);
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            propertyTemplate.addChild(properties);
        }
        propertyTemplate.addChild(FBXNode());
        objectType.addChild(propertyTemplate);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    if (hasAnimtion) {
        FBXNode objectType("ObjectType");
        objectType.addProperty("AnimationLayer");
        objectType.addPropertyNode("Count", (int32_t)animationLayerCount);
        FBXNode propertyTemplate("PropertyTemplate");
        propertyTemplate.addProperty("FbxAnimLayer");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("Weight");
                p.addProperty("Number");
                p.addProperty("");
                p.addProperty("A");
                p.addProperty((double)100.000000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Mute");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Solo");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Lock");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("Color");
                p.addProperty("ColorRGB");
                p.addProperty("Color");
                p.addProperty("");
                p.addProperty((double)0.800000);
                p.addProperty((double)0.800000);
                p.addProperty((double)0.800000);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("BlendMode");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("RotationAccumulationMode");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("ScaleAccumulationMode");
                p.addProperty("enum");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)0);
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("BlendModeBypass");
                p.addProperty("ULongLong");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int64_t)0);
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            propertyTemplate.addChild(properties);
        }
        propertyTemplate.addChild(FBXNode());
        objectType.addChild(propertyTemplate);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    if (hasAnimtion) {
        FBXNode objectType("ObjectType");
        objectType.addProperty("AnimationCurveNode");
        objectType.addPropertyNode("Count", (int32_t)animationCurveNodeCount);
        FBXNode propertyTemplate("PropertyTemplate");
        propertyTemplate.addProperty("FbxAnimCurveNode");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("d");
                p.addProperty("Compound");
                p.addProperty("");
                p.addProperty("");
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            propertyTemplate.addChild(properties);
        }
        propertyTemplate.addChild(FBXNode());
        objectType.addChild(propertyTemplate);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    if (hasAnimtion) {
        FBXNode objectType("ObjectType");
        objectType.addProperty("AnimationCurve");
        objectType.addPropertyNode("Count", (int32_t)animationCurveCount);
        objectType.addChild(FBXNode());
        definitions.addChild(objectType);
    }
    definitions.addChild(FBXNode());
    m_fbxDocument.nodes.push_back(definitions);
}

FbxFileWriter::FbxFileWriter(dust3d::Object &object,
        const QString &filename,
        QImage *textureImage,
        QImage *normalImage,
        QImage *metalnessImage,
        QImage *roughnessImage,
        QImage *ambientOcclusionImage) :
    m_filename(filename),
    m_baseName(QFileInfo(m_filename).baseName())
{
    createFbxHeader();
    createFileId();
    createCreationTime();
    createCreator();
    createGlobalSettings();
    createDocuments();
    createReferences();
    
    FBXNode connections("Connections");
    
    size_t deformerCount = 0;
    
    FBXNode geometry("Geometry");
    int64_t geometryId = m_next64Id++;
    geometry.addProperty(geometryId);
    geometry.addProperty(std::vector<uint8_t>({'u','n','a','m','e','d','m','e','s','h',0,1,'G','e','o','m','e','t','r','y'}), 'S');
    geometry.addProperty("Mesh");
    std::vector<double> positions;
    for (const auto &vertex: object.vertices) {
        positions.push_back((double)vertex.x());
        positions.push_back((double)vertex.y());
        positions.push_back((double)vertex.z());
    }
    std::vector<int32_t> indices;
    for (const auto &triangle: object.triangles) {
        indices.push_back(triangle[0]);
        indices.push_back(triangle[1]);
        indices.push_back(triangle[2] ^ -1);
    }
    FBXNode layerElementNormal("LayerElementNormal");
    const auto triangleVertexNormals = object.triangleVertexNormals();
    if (nullptr != triangleVertexNormals) {
        layerElementNormal.addProperty((int32_t)0);
        layerElementNormal.addPropertyNode("Version", (int32_t)101);
        layerElementNormal.addPropertyNode("Name", "");
        layerElementNormal.addPropertyNode("MappingInformationType", "ByPolygonVertex");
        layerElementNormal.addPropertyNode("ReferenceInformationType", "Direct");
        std::vector<double> normals;
        for (decltype(triangleVertexNormals->size()) i = 0; i < triangleVertexNormals->size(); ++i) {
            for (size_t j = 0; j < 3; ++j) {
                const auto &n = (*triangleVertexNormals)[i][j];
                normals.push_back((double)n.x());
                normals.push_back((double)n.y());
                normals.push_back((double)n.z());
            }
        }
        layerElementNormal.addPropertyNode("Normals", normals);
        layerElementNormal.addChild(FBXNode());
    }
    FBXNode layerElementUv("LayerElementUV");
    const auto triangleVertexUvs = object.triangleVertexUvs();
    if (nullptr != triangleVertexUvs) {
        layerElementUv.addProperty((int32_t)0);
        layerElementUv.addPropertyNode("Version", (int32_t)101);
        layerElementUv.addPropertyNode("Name", "default");
        layerElementUv.addPropertyNode("MappingInformationType", "ByPolygonVertex");
        layerElementUv.addPropertyNode("ReferenceInformationType", "Direct");
        std::vector<double> uvs;
        //std::vector<int32_t> uvIndices;
        for (decltype(triangleVertexUvs->size()) i = 0; i < triangleVertexUvs->size(); ++i) {
            for (size_t j = 0; j < 3; ++j) {
                const auto &uv = (*triangleVertexUvs)[i][j];
                uvs.push_back((double)uv.x());
                uvs.push_back((double)1.0 - uv.y());
                //uvIndices.push_back(uvIndices.size());
            }
        }
        layerElementUv.addPropertyNode("UV", uvs);
        //layerElementUv.addPropertyNode("UVIndex", uvIndices);
        layerElementUv.addChild(FBXNode());
    }
    FBXNode layerElementMaterial("LayerElementMaterial");
    layerElementMaterial.addProperty((int32_t)0);
    layerElementMaterial.addPropertyNode("Version", (int32_t)101);
    layerElementMaterial.addPropertyNode("Name", "");
    layerElementMaterial.addPropertyNode("MappingInformationType", "AllSame");
    layerElementMaterial.addPropertyNode("ReferenceInformationType", "IndexToDirect");
    std::vector<int32_t> materials = {(int32_t)0};
    layerElementMaterial.addPropertyNode("Materials", materials);
    layerElementMaterial.addChild(FBXNode());
    FBXNode layer("Layer");
    layer.addProperty((int32_t)0);
    layer.addPropertyNode("Version", (int32_t)100);
    if (nullptr != triangleVertexNormals) {
        FBXNode layerElement("LayerElement");
        layerElement.addPropertyNode("Type", "LayerElementNormal");
        layerElement.addPropertyNode("TypedIndex", (int32_t)0);
        layerElement.addChild(FBXNode());
        layer.addChild(layerElement);
    }
    {
        FBXNode layerElement("LayerElement");
        layerElement.addPropertyNode("Type", "LayerElementMaterial");
        layerElement.addPropertyNode("TypedIndex", (int32_t)0);
        layerElement.addChild(FBXNode());
        layer.addChild(layerElement);
    }
    if (nullptr != triangleVertexUvs) {
        FBXNode layerElement("LayerElement");
        layerElement.addPropertyNode("Type", "LayerElementUV");
        layerElement.addPropertyNode("TypedIndex", (int32_t)0);
        layerElement.addChild(FBXNode());
        layer.addChild(layerElement);
    }
    layer.addChild(FBXNode());
    geometry.addPropertyNode("GeometryVersion", (int32_t)124);
    geometry.addPropertyNode("Vertices", positions);
    geometry.addPropertyNode("PolygonVertexIndex", indices);
    if (nullptr != triangleVertexNormals)
        geometry.addChild(layerElementNormal);
    geometry.addChild(layerElementMaterial);
    if (nullptr != triangleVertexUvs)
        geometry.addChild(layerElementUv);
    geometry.addChild(layer);
    geometry.addChild(FBXNode());
    
    int64_t modelId = m_next64Id++;
    FBXNode model("Model");
    model.addProperty(modelId);
    model.addProperty(std::vector<uint8_t>({'u','n','a','m','e','d',0,1,'M','o','d','e','l'}), 'S');
    model.addProperty("Mesh");
    model.addPropertyNode("Version", (int32_t)232);
    {
        FBXNode properties("Properties70");
        {
            FBXNode p("P");
            p.addProperty("Lcl Rotation");
            p.addProperty("Lcl Rotation");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Lcl Scaling");
            p.addProperty("Lcl Scaling");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)1.000000);
            p.addProperty((double)1.000000);
            p.addProperty((double)1.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("DefaultAttributeIndex");
            p.addProperty("int");
            p.addProperty("Integer");
            p.addProperty("");
            p.addProperty((int32_t)0);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("InheritType");
            p.addProperty("enum");
            p.addProperty("");
            p.addProperty("");
            p.addProperty((int32_t)1);
            properties.addChild(p);
        }
        /*
        {
            FBXNode p("P");
            p.addProperty("currentUVSet");
            p.addProperty("KString");
            p.addProperty("");
            p.addProperty("U");
            p.addProperty("default");
            properties.addChild(p);
        }*/
        properties.addChild(FBXNode());
        model.addChild(properties);
    }
    model.addPropertyNode("MultiLayer", (int32_t)0);
    model.addPropertyNode("MultiTake", (int32_t)0);
    model.addPropertyNode("Shading", (bool)true);
    model.addPropertyNode("Culling", "CullingOff");
    model.addChild(FBXNode());
    
    FBXNode pose("Pose");
    int64_t poseId = 0;
    std::vector<FBXNode> deformers;
    std::vector<int64_t> deformerIds;
    std::vector<FBXNode> limbNodes;
    std::vector<int64_t> limbNodeIds;
    std::vector<FBXNode> nodeAttributes;
    std::vector<int64_t> nodeAttributeIds;
    int64_t skinId = 0;
    int64_t armatureId = 0;
    
    if (deformerCount > 0)
    {
        poseId = m_next64Id++;
        pose.addProperty(poseId);
        pose.addProperty(std::vector<uint8_t>({'u','n','a','m','e','d',0,1,'P','o','s','e'}), 'S');
        pose.addProperty("BindPose");
        pose.addPropertyNode("Type", "BindPose");
        pose.addPropertyNode("Version", (int32_t)100);
        pose.addPropertyNode("NbPoseNodes", (int32_t)(1 + deformerCount)); // +1 for model
        {
            FBXNode poseNode("PoseNode");
            poseNode.addPropertyNode("Node", (int64_t)modelId);
            poseNode.addPropertyNode("Matrix", m_identityMatrix);
            poseNode.addChild(FBXNode());
            pose.addChild(poseNode);
        }
        {
            FBXNode poseNode("PoseNode");
            poseNode.addPropertyNode("Node", (int64_t)armatureId);
            poseNode.addPropertyNode("Matrix", m_identityMatrix);
            poseNode.addChild(FBXNode());
            pose.addChild(poseNode);
        }
        pose.addChild(FBXNode());
    }
    
    size_t textureCount = 0;
    size_t videoCount = 0;
    
    std::vector<FBXNode> videos;
    std::vector<FBXNode> textures;
    
    FBXNode material("Material");
    int64_t materialId = m_next64Id++;
    material.addProperty(materialId);
    material.addProperty(std::vector<uint8_t>({'M','a','t','e','r','i','a','l',0,1,'S','t','i','n','g','r','a','y','P','B','S'}), 'S');
    material.addProperty("");
    material.addPropertyNode("Version", (int32_t)102);
    material.addPropertyNode("ShadingModel", "unknown");
    material.addPropertyNode("MultiLayer", (int32_t)0);
    {
        FBXNode properties("Properties70");
        {
            FBXNode p("P");
            p.addProperty("Maya");
            p.addProperty("Compound");
            p.addProperty("");
            p.addProperty("");
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TypeId");
            p.addProperty("int");
            p.addProperty("Integer");
            p.addProperty("");
            p.addProperty((int32_t)1166017); // Don't known what does this id mean
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TEX_global_diffuse_cube");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TEX_global_specular_cube");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TEX_brdf_lut");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|use_normal_map");
            p.addProperty("float");
            p.addProperty("");
            p.addProperty("");
            p.addProperty(normalImage ? (float)1.000000 : (float)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|uv_offset");
            p.addProperty("Vector2D");
            p.addProperty("Vector2");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|uv_scale");
            p.addProperty("Vector2D");
            p.addProperty("Vector2");
            p.addProperty("");
            p.addProperty((double)1.000000);
            p.addProperty((double)1.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TEX_normal_map");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|use_color_map");
            p.addProperty("float");
            p.addProperty("");
            p.addProperty("");
            p.addProperty(textureImage ? (float)1.000000 : (float)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TEX_color_map");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|base_color");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)Theme::white.redF());
            p.addProperty((double)Theme::white.greenF());
            p.addProperty((double)Theme::white.blueF());
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|use_metallic_map");
            p.addProperty("float");
            p.addProperty("");
            p.addProperty("");
            p.addProperty(metalnessImage ? (float)1.000000 : (float)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TEX_metallic_map");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|metallic");
            p.addProperty("float");
            p.addProperty("");
            p.addProperty((float)0.0);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|use_roughness_map");
            p.addProperty("float");
            p.addProperty("");
            p.addProperty("");
            p.addProperty(roughnessImage ? (float)1.000000 : (float)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TEX_roughness_map");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|roughness");
            p.addProperty("float");
            p.addProperty("");
            p.addProperty((float)1.0);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|use_emissive_map");
            p.addProperty("float");
            p.addProperty("");
            p.addProperty("");
            p.addProperty((float)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TEX_emissive_map");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|emissive");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|emissive_intensity");
            p.addProperty("float");
            p.addProperty("");
            p.addProperty("");
            p.addProperty((float)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TEX_metallic_map");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|use_ao_map");
            p.addProperty("float");
            p.addProperty("");
            p.addProperty("");
            p.addProperty(ambientOcclusionImage ? (float)1.000000 : (float)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Maya|TEX_ao_map");
            p.addProperty("Vector3D");
            p.addProperty("Vector");
            p.addProperty("");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        properties.addChild(FBXNode());
        material.addChild(properties);
    }
    material.addChild(FBXNode());
    
    /*
    FBXNode material("Material");
    int64_t materialId = m_next64Id++;
    material.addProperty(materialId);
    material.addProperty(std::vector<uint8_t>({'u','n','a','m','e','d',0,1,'M','a','t','e','r','i','a','l'}), 'S');
    material.addProperty("");
    material.addPropertyNode("Version", (int32_t)102);
    material.addPropertyNode("ShadingModel", "Phong");
    material.addPropertyNode("MultiLayer", (int32_t)0);
    {
        FBXNode properties("Properties70");
        {
            FBXNode p("P");
            p.addProperty("EmissiveColor");
            p.addProperty("Color");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)0.800000);
            p.addProperty((double)0.800000);
            p.addProperty((double)0.800000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("EmissiveFactor");
            p.addProperty("Number");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("AmbientColor");
            p.addProperty("Color");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("DiffuseColor");
            p.addProperty("Color");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)0.800000);
            p.addProperty((double)0.800000);
            p.addProperty((double)0.800000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("DiffuseFactor");
            p.addProperty("Number");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)0.800000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("TransparentColor");
            p.addProperty("Color");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)1.000000);
            p.addProperty((double)1.000000);
            p.addProperty((double)1.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("SpecularColor");
            p.addProperty("Color");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)1.000000);
            p.addProperty((double)1.000000);
            p.addProperty((double)1.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("SpecularFactor");
            p.addProperty("Number");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)0.250000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("Shininess");
            p.addProperty("Number");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)9.607843);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("ShininessExponent");
            p.addProperty("Number");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)9.607843);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("ReflectionColor");
            p.addProperty("Color");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)1.000000);
            p.addProperty((double)1.000000);
            p.addProperty((double)1.000000);
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("ReflectionFactor");
            p.addProperty("Number");
            p.addProperty("");
            p.addProperty("A");
            p.addProperty((double)0.000000);
            properties.addChild(p);
        }
        properties.addChild(FBXNode());
        material.addChild(properties);
    }
    material.addChild(FBXNode());
    */
    
    FBXNode implementation("Implementation");
    int64_t implementationId = m_next64Id++;
    implementation.addProperty(implementationId);
    implementation.addProperty(std::vector<uint8_t>({'I','m','p','l','e','m','e','n','t','a','t','i','o','n',0,1,'S','t','i','n','g','r','a','y','P','B','S','_','I','m','p','l','e','m','e','n','t','a','t','i','o','n'}), 'S');
    implementation.addProperty("");
    implementation.addPropertyNode("Version", (int32_t)100);
    {
        FBXNode properties("Properties70");
        {
            FBXNode p("P");
            p.addProperty("ShaderLanguage");
            p.addProperty("KString");
            p.addProperty("");
            p.addProperty("");
            p.addProperty("SFX");
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("ShaderLanguageVersion");
            p.addProperty("KString");
            p.addProperty("");
            p.addProperty("");
            p.addProperty("28");
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("RenderAPI");
            p.addProperty("KString");
            p.addProperty("");
            p.addProperty("");
            p.addProperty("SFX_PBS_SHADER");
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("RootBindingName");
            p.addProperty("KString");
            p.addProperty("");
            p.addProperty("");
            p.addProperty("root");
            properties.addChild(p);
        }
        /*
        {
            QString shaderGraph = ModelShaderProgram::loadShaderSource(":/shaders/stingraypbs.shadergraph");
            const auto &shaderGraphByteArray = shaderGraph.toUtf8();
            std::vector<uint8_t> content(shaderGraphByteArray.begin(), shaderGraphByteArray.end());
            FBXNode p("P");
            p.addProperty("ShaderGraph");
            p.addProperty("Blob");
            p.addProperty("");
            p.addProperty("");
            p.addProperty(content, 'R');
            properties.addChild(p);
        }
        */
        properties.addChild(FBXNode());
        implementation.addChild(properties);
    }
    implementation.addChild(FBXNode());
    
    FBXNode bindingTable("BindingTable");
    int64_t bindingTableId = m_next64Id++;
    bindingTable.addProperty(bindingTableId);
    bindingTable.addProperty(std::vector<uint8_t>({'B','i','n','d','i','n','g','T','a','b','l','e',0,1,'S','t','i','n','g','r','a','y','P','B','S','_','B','i','n','d','i','n','g','T','a','b','l','e'}), 'S');
    bindingTable.addProperty("");
    bindingTable.addPropertyNode("Version", (int32_t)100);
    {
        FBXNode properties("Properties70");
        {
            FBXNode p("P");
            p.addProperty("TargetName");
            p.addProperty("KString");
            p.addProperty("");
            p.addProperty("");
            p.addProperty("root");
            properties.addChild(p);
        }
        {
            FBXNode p("P");
            p.addProperty("TargetType");
            p.addProperty("KString");
            p.addProperty("");
            p.addProperty("");
            p.addProperty("shader");
            properties.addChild(p);
        }
        properties.addChild(FBXNode());
        bindingTable.addChild(properties);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|use_metallic_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("use_metallic_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|base_color");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("base_color");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|use_ao_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("use_ao_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|TEX_emissive_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("TEX_emissive_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|TEX_metallic_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("TEX_metallic_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|TEX_ao_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("TEX_ao_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|uv_offset");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("uv_offset");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|emissive_intensity");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("emissive_intensity");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|metallic");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("metallic");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|TEX_global_specular_cube");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("TEX_global_specular_cube");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|use_roughness_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("use_roughness_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|use_normal_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("use_normal_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|use_color_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("use_color_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|emissive");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("emissive");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|use_emissive_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("use_emissive_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|uv_scale");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("uv_scale");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|TEX_global_diffuse_cube");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("TEX_global_diffuse_cube");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|TEX_roughness_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("TEX_roughness_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|roughness");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("roughness");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|TEX_brdf_lut");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("TEX_brdf_lut");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|TEX_color_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("TEX_color_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    {
        FBXNode entry("Entry");
        entry.addProperty("Maya|TEX_normal_map");
        entry.addProperty("FbxPropertyEntry");
        entry.addProperty("TEX_normal_map");
        entry.addProperty("FbxSemanticEntry");
        bindingTable.addChild(entry);
    }
    bindingTable.addChild(FBXNode());
    
    auto addTexture = [&](const QImage *image, const std::vector<uint8_t> &clipName, const std::vector<uint8_t> &textureName, const QString &filename, const QString &propertyName){
        FBXNode video("Video");
        int64_t videoId = m_next64Id++;
        video.addProperty(videoId);
        video.addProperty(clipName, 'S');
        video.addProperty("Clip");
        video.addPropertyNode("Type", "Clip");
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("Path");
                p.addProperty("KString");
                p.addProperty("XRefUrl");
                p.addProperty("");
                p.addProperty(filename.toUtf8().constData());
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            video.addChild(properties);
        }
        video.addPropertyNode("UseMipMap", (int32_t)0);
        video.addPropertyNode("RelativeFilename", filename.toUtf8().constData());
        video.addPropertyNode("FileName", filename.toUtf8().constData());
        QByteArray pngByteArray;
        QBuffer buffer(&pngByteArray);
        image->save(&buffer, "PNG");
        std::vector<uint8_t> content(pngByteArray.begin(), pngByteArray.end());
        video.addPropertyNode("Content", content, 'R');
        video.addChild(FBXNode());
        videos.push_back(video);
        videoCount++;
        
        FBXNode texture("Texture");
        int64_t textureId = m_next64Id++;
        texture.addProperty(textureId);
        texture.addProperty(clipName, 'S');
        texture.addProperty("");
        texture.addPropertyNode("Type", "TextureVideoClip");
        texture.addPropertyNode("Version", (int32_t)202);
        texture.addPropertyNode("TextureName", textureName, 'S');
        {
            FBXNode properties("Properties70");
            {
                FBXNode p("P");
                p.addProperty("UVSet");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("default");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("UseMaterial");
                p.addProperty("bool");
                p.addProperty("");
                p.addProperty("");
                p.addProperty((int32_t)1);
                properties.addChild(p);
            }
            properties.addChild(FBXNode());
            texture.addChild(properties);
        }
        texture.addPropertyNode("Media", clipName, 'S');
        texture.addPropertyNode("RelativeFilename", filename.toUtf8().constData());
        texture.addPropertyNode("FileName", filename.toUtf8().constData());
        {
            FBXNode modelUVTranslation("ModelUVTranslation");
            modelUVTranslation.addProperty((double)0);
            modelUVTranslation.addProperty((double)0);
            texture.addChild(modelUVTranslation);
        }
        {
            FBXNode modelUVScaling("ModelUVScaling");
            modelUVScaling.addProperty((double)1);
            modelUVScaling.addProperty((double)1);
            texture.addChild(modelUVScaling);
        }
        texture.addPropertyNode("Texture_Alpha_Source", "None");
        {
            FBXNode modelUVScaling("Cropping");
            modelUVScaling.addProperty((int32_t)0);
            modelUVScaling.addProperty((int32_t)0);
            modelUVScaling.addProperty((int32_t)0);
            modelUVScaling.addProperty((int32_t)0);
            texture.addChild(modelUVScaling);
        }
        texture.addChild(FBXNode());
        textures.push_back(texture);
        textureCount++;
        
        {
            FBXNode p("C");
            p.addProperty("OO");
            p.addProperty(videoId);
            p.addProperty(textureId);
            connections.addChild(p);
        }
        {
            FBXNode p("C");
            p.addProperty("OP");
            p.addProperty(textureId);
            p.addProperty(materialId);
            p.addProperty(propertyName.toUtf8().constData());
            connections.addChild(p);
        }
    };
    if (nullptr != textureImage) {
        addTexture(textureImage,
            std::vector<uint8_t>({'V','i','d','e','o',0,1,'B','a','s','e','C','o','l','o','r'}),
            std::vector<uint8_t>({'T','e','x','t','u','r','e',0,1,'B','a','s','e','C','o','l','o','r'}),
            m_baseName + "_color.png",
            "Maya|TEX_color_map");
    }
    if (nullptr != normalImage) {
        addTexture(normalImage,
            std::vector<uint8_t>({'V','i','d','e','o',0,1,'N','o','r','m','a','l'}),
            std::vector<uint8_t>({'T','e','x','t','u','r','e',0,1,'N','o','r','m','a','l'}),
            m_baseName + "_normal.png",
            "Maya|TEX_normal_map");
    }
    if (nullptr != metalnessImage) {
        addTexture(metalnessImage,
            std::vector<uint8_t>({'V','i','d','e','o',0,1,'M','e','t','a','l','l','i','c'}),
            std::vector<uint8_t>({'T','e','x','t','u','r','e',0,1,'M','e','t','a','l','l','i','c'}),
            m_baseName + "_metallic.png",
            "Maya|TEX_metallic_map");
    }
    if (nullptr != roughnessImage) {
        addTexture(roughnessImage,
            std::vector<uint8_t>({'V','i','d','e','o',0,1,'R','o','u','g','h','n','e','s','s'}),
            std::vector<uint8_t>({'T','e','x','t','u','r','e',0,1,'R','o','u','g','h','n','e','s','s'}),
            m_baseName + "_roughness.png",
            "Maya|TEX_roughness_map");
    }
    if (nullptr != ambientOcclusionImage) {
        addTexture(ambientOcclusionImage,
            std::vector<uint8_t>({'V','i','d','e','o',0,1,'A','o'}),
            std::vector<uint8_t>({'T','e','x','t','u','r','e',0,1,'A','o'}),
            m_baseName + "_ao.png",
            "Maya|TEX_ao_map");
    }
    
    bool hasAnimation = false;
    size_t animationStackCount = 0;
    size_t animationLayerCount = 0;
    size_t animationCurveNodeCount = 0;
    size_t animationCurveCount = 0;
    
    std::vector<FBXNode> animationStacks;
    std::vector<FBXNode> animationLayers;
    std::vector<FBXNode> animationCurveNodes;
    std::vector<FBXNode> animationCurves;

    createDefinitions(deformerCount,
        textureCount, videoCount,
        hasAnimation,
        animationStackCount, animationLayerCount, animationCurveNodeCount, animationCurveCount);
    
    FBXNode objects("Objects");
    objects.addChild(geometry);
    objects.addChild(model);
    for (const auto &limbNode: limbNodes) {
        objects.addChild(limbNode);
    }
    if (deformerCount > 0)
        objects.addChild(pose);
    objects.addChild(material);
    objects.addChild(implementation);
    objects.addChild(bindingTable);
    if (textureCount > 0) {
        for (const auto &texture: textures) {
            objects.addChild(texture);
        }
    }
    if (videoCount > 0) {
        for (const auto &video: videos) {
            objects.addChild(video);
        }
    }
    for (const auto &deformer: deformers) {
        objects.addChild(deformer);
    }
    for (const auto &nodeAttribute: nodeAttributes) {
        objects.addChild(nodeAttribute);
    }
    if (hasAnimation) {
        for (const auto &animationStack: animationStacks) {
            objects.addChild(animationStack);
        }
        for (const auto &animationLayer: animationLayers) {
            objects.addChild(animationLayer);
        }
        for (const auto &animationCurveNode: animationCurveNodes) {
            objects.addChild(animationCurveNode);
        }
        for (const auto &animationCurve: animationCurves) {
            objects.addChild(animationCurve);
        }
    }
    objects.addChild(FBXNode());
    m_fbxDocument.nodes.push_back(objects);
    
    {
        FBXNode p("C");
        p.addProperty("OO");
        p.addProperty(modelId);
        p.addProperty((int64_t)0);
        connections.addChild(p);
    }
    if (armatureId > 0)
    {
        FBXNode p("C");
        p.addProperty("OO");
        p.addProperty(armatureId);
        p.addProperty((int64_t)0);
        connections.addChild(p);
    }
    {
        FBXNode p("C");
        p.addProperty("OO");
        p.addProperty(geometryId);
        p.addProperty(modelId);
        connections.addChild(p);
    }
    if (skinId > 0)
    {
        FBXNode p("C");
        p.addProperty("OO");
        p.addProperty(skinId);
        p.addProperty(geometryId);
        connections.addChild(p);
    }
    for (size_t i = 0; i < limbNodeIds.size(); ++i) {
        FBXNode p("C");
        p.addProperty("OO");
        p.addProperty(limbNodeIds[i]);
        p.addProperty(deformerIds[i]);
        connections.addChild(p);
    }
    for (size_t i = 0; i < nodeAttributeIds.size(); ++i) {
        FBXNode p("C");
        p.addProperty("OO");
        p.addProperty(nodeAttributeIds[i]);
        p.addProperty(limbNodeIds[i]);
        connections.addChild(p);
    }
    {
        FBXNode p("C");
        p.addProperty("OO");
        p.addProperty(materialId);
        p.addProperty(implementationId);
        connections.addChild(p);
    }
    {
        FBXNode p("C");
        p.addProperty("OO");
        p.addProperty(bindingTableId);
        p.addProperty(implementationId);
        connections.addChild(p);
    }
    {
        FBXNode p("C");
        p.addProperty("OO");
        p.addProperty(materialId);
        p.addProperty(modelId);
        connections.addChild(p);
    }
    connections.addChild(FBXNode());
    m_fbxDocument.nodes.push_back(connections);
    
    createTakes();
}

void FbxFileWriter::createTakes()
{
    FBXNode takes("Takes");
    takes.addPropertyNode("Current", "");
    takes.addChild(FBXNode());
    m_fbxDocument.nodes.push_back(takes);
}

bool FbxFileWriter::save()
{
    //m_fbxDocument.print();
    m_fbxDocument.write(m_filename.toStdString());
    return true;
}

std::vector<double> FbxFileWriter::matrixToVector(const QMatrix4x4 &matrix)
{
    std::vector<double> vec;
    for (size_t col = 0; col < 4; ++col) {
        const auto &line = matrix.column(col);
        vec.push_back(line.x());
        vec.push_back(line.y());
        vec.push_back(line.z());
        vec.push_back(line.w());
    }
    return vec;
}

int64_t FbxFileWriter::secondsToKtime(double seconds)
{
    return (int64_t)(seconds * 46186158000);
}
