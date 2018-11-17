#include <fbxnode.h>
#include <fbxproperty.h>
#include <QDateTime>
#include <QtMath>
#include "fbxfile.h"
#include "version.h"
#include "jointnodetree.h"
#include "util.h"

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

FbxFileWriter::FbxFileWriter(Outcome &outcome,
        const std::vector<RiggerBone> *resultRigBones,
        const std::map<int, RiggerVertexWeights> *resultRigWeights,
        const QString &filename,
        const std::vector<std::pair<QString, std::vector<std::pair<float, JointNodeTree>>>> *motions) :
    m_filename(filename)
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
    if (resultRigBones && !resultRigBones->empty())
        deformerCount = 1 + resultRigBones->size(); // 1 for the root Skin deformer
    
    JointNodeTree jointNodeTree(resultRigBones);
    const auto &boneNodes = jointNodeTree.nodes();
    
    FBXNode geometry("Geometry");
    int64_t geometryId = m_next64Id++;
    geometry.addProperty(geometryId);
    geometry.addProperty(std::vector<uint8_t>({'u','n','a','m','e','d','m','e','s','h',0,1,'G','e','o','m','e','t','r','y'}), 'S');
    geometry.addProperty("Mesh");
    std::vector<double> positions;
    for (const auto &vertex: outcome.vertices) {
        positions.push_back((double)vertex.x());
        positions.push_back((double)vertex.y());
        positions.push_back((double)vertex.z());
    }
    std::vector<int32_t> indices;
    for (const auto &triangle: outcome.triangles) {
        indices.push_back(triangle[0]);
        indices.push_back(triangle[1]);
        indices.push_back(triangle[2] ^ -1);
    }
    FBXNode layerElementNormal("LayerElementNormal");
    const auto triangleVertexNormals = outcome.triangleVertexNormals();
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
    {
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
    layer.addChild(FBXNode());
    geometry.addPropertyNode("GeometryVersion", (int32_t)124);
    geometry.addPropertyNode("Vertices", positions);
    geometry.addPropertyNode("PolygonVertexIndex", indices);
    if (nullptr != triangleVertexNormals)
        geometry.addChild(layerElementNormal);
    geometry.addChild(layerElementMaterial);
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
    if (resultRigBones && !resultRigBones->empty()) {
        std::vector<std::pair<std::vector<int32_t>, std::vector<double>>> bindPerBone(resultRigBones->size());
        if (resultRigWeights && !resultRigWeights->empty()) {
            for (const auto &item: *resultRigWeights) {
                for (int i = 0; i < 4; ++i) {
                    const auto &boneIndex = item.second.boneIndices[i];
                    Q_ASSERT(boneIndex < bindPerBone.size());
                    if (0 == boneIndex)
                        break;
                    bindPerBone[boneIndex].first.push_back(item.first);
                    bindPerBone[boneIndex].second.push_back(item.second.boneWeights[i]);
                }
            }
        }
    
        {
            skinId = m_next64Id++;
            deformerIds.push_back(skinId);
            
            deformers.push_back(FBXNode("Deformer"));
            FBXNode &deformer = deformers.back();
            deformer.addProperty(skinId);
            deformer.addProperty(std::vector<uint8_t>({'A','r','m','a','t','u','r','e',0,1,'D','e','f','o','r','m','e','r'}), 'S');
            deformer.addProperty("Skin");
            deformer.addPropertyNode("Version", (int32_t)101);
            deformer.addPropertyNode("Link_DeformAcuracy", (double)50.000000);
            deformer.addChild(FBXNode());
        }
        
        {
            armatureId = m_next64Id++;
            limbNodeIds.push_back(armatureId);
            
            limbNodes.push_back(FBXNode("Model"));
            FBXNode &limbNode = limbNodes.back();
            limbNode.addProperty(armatureId);
            limbNode.addProperty(std::vector<uint8_t>({'A','r','m','a','t','u','r','e',0,1,'M','o','d','e','l'}), 'S');
            limbNode.addProperty("Null");
            limbNode.addPropertyNode("Version", (int32_t)232);
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
                properties.addChild(FBXNode());
                limbNode.addChild(properties);
            }
            limbNode.addPropertyNode("MultiLayer", (int32_t)0);
            limbNode.addPropertyNode("MultiTake", (int32_t)0);
            limbNode.addPropertyNode("Shading", (bool)true);
            limbNode.addPropertyNode("Culling", "CullingOff");
            limbNode.addChild(FBXNode());
        }
        
        {
            int64_t nodeAttributeId = m_next64Id++;
            nodeAttributeIds.push_back(nodeAttributeId);
            
            nodeAttributes.push_back(FBXNode("NodeAttribute"));
            FBXNode &nodeAttribute = nodeAttributes.back();
            nodeAttribute.addProperty(nodeAttributeId);
            nodeAttribute.addProperty(std::vector<uint8_t>({'A','r','m','a','t','u','r','e',0,1,'N','o','d','e','A','t','t','r','i','b','u','t','e'}), 'S');
            nodeAttribute.addProperty("Null");
            nodeAttribute.addPropertyNode("TypeFlags", "Null");
            {
                FBXNode properties("Properties70");
                {
                    FBXNode p("P");
                    p.addProperty("Color");
                    p.addProperty("ColorRGB");
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
                    p.addProperty("Size");
                    p.addProperty("double");
                    p.addProperty("Number");
                    p.addProperty("");
                    p.addProperty("A");
                    p.addProperty((double)100.000000);
                    properties.addChild(p);
                }
                {
                    FBXNode p("P");
                    p.addProperty("Look");
                    p.addProperty("enum");
                    p.addProperty("");
                    p.addProperty("");
                    p.addProperty((int32_t)1);
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
                properties.addChild(FBXNode());
                nodeAttribute.addChild(properties);
            }
            nodeAttribute.addPropertyNode("MultiLayer", (int32_t)0);
            nodeAttribute.addPropertyNode("MultiTake", (int32_t)0);
            nodeAttribute.addPropertyNode("Shading", (bool)true);
            nodeAttribute.addPropertyNode("Culling", "CullingOff");
            nodeAttribute.addChild(FBXNode());
        }
        
        for (size_t i = 0; i < resultRigBones->size(); ++i) {
            const auto &bone = (*resultRigBones)[i];
            const auto &jointNode = boneNodes[i];
            
            {
                int64_t clusterId = m_next64Id++;
                deformerIds.push_back(clusterId);
                
                deformers.push_back(FBXNode("Deformer"));
                FBXNode &deformer = deformers.back();
                deformer.addProperty(clusterId);
                std::vector<uint8_t> name;
                QString clusterName = APP_NAME + QString(":") + bone.name;
                for (const auto &c: clusterName) {
                    name.push_back((uint8_t)c.toLatin1());
                }
                name.push_back(0);
                name.push_back(1);
                name.push_back('S');
                name.push_back('u');
                name.push_back('b');
                name.push_back('D');
                name.push_back('e');
                name.push_back('f');
                name.push_back('o');
                name.push_back('r');
                name.push_back('m');
                name.push_back('e');
                name.push_back('r');
                deformer.addProperty(name, 'S');
                deformer.addProperty("Cluster");
                deformer.addPropertyNode("Version", (int32_t)100);
                FBXNode userData("UserData");
                userData.addProperty("");
                userData.addProperty("");
                deformer.addChild(userData);
                deformer.addPropertyNode("Indexes", bindPerBone[i].first);
                deformer.addPropertyNode("Weights", bindPerBone[i].second);
                deformer.addPropertyNode("Transform", matrixToVector(jointNode.transformMatrix.inverted()));
                deformer.addPropertyNode("TransformLink", matrixToVector(jointNode.transformMatrix));
                deformer.addPropertyNode("TransformAssociateModel", m_identityMatrix);
                deformer.addChild(FBXNode());
            }
            
            {
                int64_t limbNodeId = m_next64Id++;
                limbNodeIds.push_back(limbNodeId);
                
                limbNodes.push_back(FBXNode("Model"));
                FBXNode &limbNode = limbNodes.back();
                limbNode.addProperty(limbNodeId);
                std::vector<uint8_t> name;
                QString limbNodeName = APP_NAME + QString(":") + bone.name;
                for (const auto &c: limbNodeName) {
                    name.push_back((uint8_t)c.toLatin1());
                }
                name.push_back(0);
                name.push_back(1);
                name.push_back('M');
                name.push_back('o');
                name.push_back('d');
                name.push_back('e');
                name.push_back('l');
                limbNode.addProperty(name, 'S');
                limbNode.addProperty("LimbNode");
                limbNode.addPropertyNode("Version", (int32_t)232);
                {
                    FBXNode properties("Properties70");
                    {
                        FBXNode p("P");
                        p.addProperty("Lcl Translation");
                        p.addProperty("Lcl Translation");
                        p.addProperty("");
                        p.addProperty("A");
                        p.addProperty((double)jointNode.translation.x());
                        p.addProperty((double)jointNode.translation.y());
                        p.addProperty((double)jointNode.translation.z());
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
                    properties.addChild(FBXNode());
                    limbNode.addChild(properties);
                }
                limbNode.addPropertyNode("MultiLayer", (int32_t)0);
                limbNode.addPropertyNode("MultiTake", (int32_t)0);
                limbNode.addPropertyNode("Shading", (bool)true);
                limbNode.addPropertyNode("Culling", "CullingOff");
                limbNode.addChild(FBXNode());
            }
            
            {
                int64_t nodeAttributeId = m_next64Id++;
                nodeAttributeIds.push_back(nodeAttributeId);
                
                nodeAttributes.push_back(FBXNode("NodeAttribute"));
                FBXNode &nodeAttribute = nodeAttributes.back();
                nodeAttribute.addProperty(nodeAttributeId);
                std::vector<uint8_t> name;
                QString nodeAttributeName = APP_NAME + QString(":") + bone.name;
                for (const auto &c: nodeAttributeName) {
                    name.push_back((uint8_t)c.toLatin1());
                }
                name.push_back(0);
                name.push_back(1);
                name.push_back('N');
                name.push_back('o');
                name.push_back('d');
                name.push_back('e');
                name.push_back('A');
                name.push_back('t');
                name.push_back('t');
                name.push_back('r');
                name.push_back('i');
                name.push_back('b');
                name.push_back('u');
                name.push_back('t');
                name.push_back('e');
                nodeAttribute.addProperty(name, 'S');
                nodeAttribute.addProperty("LimbNode");
                nodeAttribute.addPropertyNode("TypeFlags", "Skeleton");
                {
                    FBXNode properties("Properties70");
                    {
                        FBXNode p("P");
                        p.addProperty("Size");
                        p.addProperty("double");
                        p.addProperty("Number");
                        p.addProperty("");
                        p.addProperty("A");
                        p.addProperty((double)3.300000);
                        properties.addChild(p);
                    }
                    properties.addChild(FBXNode());
                    nodeAttribute.addChild(properties);
                }
                nodeAttribute.addPropertyNode("MultiLayer", (int32_t)0);
                nodeAttribute.addPropertyNode("MultiTake", (int32_t)0);
                nodeAttribute.addPropertyNode("Shading", (bool)true);
                nodeAttribute.addPropertyNode("Culling", "CullingOff");
                nodeAttribute.addChild(FBXNode());
            }
        }
    }
    
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
        for (size_t i = 0; i < boneNodes.size(); i++) {
            const auto &boneNode = boneNodes[i];
            FBXNode poseNode("PoseNode");
            poseNode.addPropertyNode("Node", (int64_t)limbNodeIds[1 + i]);
            poseNode.addPropertyNode("Matrix", matrixToVector(boneNode.transformMatrix));
            poseNode.addChild(FBXNode());
            pose.addChild(poseNode);
        }
        pose.addChild(FBXNode());
    }
    
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
    
    bool hasAnimation = nullptr != motions && !motions->empty();
    size_t animationStackCount = 0;
    size_t animationLayerCount = 0;
    size_t animationCurveNodeCount = 0;
    size_t animationCurveCount = 0;
    
    std::vector<FBXNode> animationStacks;
    std::vector<FBXNode> animationLayers;
    std::vector<FBXNode> animationCurveNodes;
    std::vector<FBXNode> animationCurves;
    
    if (hasAnimation) {
        std::set<int> rotatedJoints;
        std::set<int> translatedJoints;
    
        for (int animationIndex = 0; animationIndex < (int)motions->size(); ++animationIndex) {
            const auto &motion = (*motions)[animationIndex];
            
            FBXNode animationStack("AnimationStack");
            int64_t animationStackId = m_next64Id++;
            animationStack.addProperty(animationStackId);
            {
                std::vector<uint8_t> name;
                auto stackName = motion.first.toUtf8();
                for (const auto &c: stackName) {
                    name.push_back((uint8_t)c);
                }
                name.push_back(0);
                name.push_back(1);
                name.push_back('A');
                name.push_back('n');
                name.push_back('i');
                name.push_back('m');
                name.push_back('S');
                name.push_back('t');
                name.push_back('a');
                name.push_back('c');
                name.push_back('k');
                animationStack.addProperty(name, 'S');
            }
            animationStack.addProperty("");
            {
                FBXNode properties("Properties70");
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
                    p.addProperty("ReferenceStop");
                    p.addProperty("KTime");
                    p.addProperty("Time");
                    p.addProperty("");
                    p.addProperty((int64_t)0);
                    properties.addChild(p);
                }
                properties.addChild(FBXNode());
                animationStack.addChild(properties);
            }
            animationStack.addChild(FBXNode());
            animationStacks.push_back(animationStack);
            
            FBXNode animationLayer("AnimationLayer");
            int64_t animationLayerId = m_next64Id++;
            animationLayer.addProperty((int64_t)animationLayerId);
            {
                std::vector<uint8_t> name;
                auto layerName = motion.first.toUtf8();
                for (const auto &c: layerName) {
                    name.push_back((uint8_t)c);
                }
                name.push_back(0);
                name.push_back(1);
                name.push_back('A');
                name.push_back('n');
                name.push_back('i');
                name.push_back('m');
                name.push_back('L');
                name.push_back('a');
                name.push_back('y');
                name.push_back('e');
                name.push_back('r');
                animationLayer.addProperty(name, 'S');
            }
            animationLayer.addProperty("");
            animationLayer.addChild(FBXNode());
            animationLayers.push_back(animationLayer);
            
            {
                FBXNode p("C");
                p.addProperty("OO");
                p.addProperty(animationLayerId);
                p.addProperty(animationStackId);
                connections.addChild(p);
            }
            
            for (const auto &keyframe: motion.second) {
                for (int i = 0; i < (int)keyframe.second.nodes().size() && i < (int)boneNodes.size(); ++i) {
                    const auto &src = boneNodes[i];
                    const auto &dest = keyframe.second.nodes()[i];
                    if (!qFuzzyCompare(src.rotation, dest.rotation))
                        rotatedJoints.insert(i);
                    if (!qFuzzyCompare(src.translation, dest.translation))
                        translatedJoints.insert(i);
                }
            }
            
            for (const auto &jointIndex: translatedJoints) {
                int64_t animationCurveIds[3];
                std::vector<int64_t> ktimes;
                std::vector<float> values[3];
                for (int curveIndex = 0; curveIndex < 3; ++curveIndex) {
                    animationCurveIds[curveIndex] = m_next64Id++;
                }
                double timePoint = 0;
                for (int frame = 0; frame < (int)motion.second.size(); frame++) {
                    const auto &keyframe = motion.second[frame];
                    const auto &translation = keyframe.second.nodes()[jointIndex].translation;
                    double x = translation.x();
                    double y = translation.y();
                    double z = translation.z();
                    
                    values[0].push_back(x);
                    values[1].push_back(y);
                    values[2].push_back(z);
                    ktimes.push_back(secondsToKtime(timePoint));
                    timePoint += keyframe.first;
                    
                    FBXNode animationCurveNode("AnimationCurveNode");
                    int64_t animationCurveNodeId = m_next64Id++;
                    animationCurveNode.addProperty(animationCurveNodeId);
                    animationCurveNode.addProperty(std::vector<uint8_t>({'T',0,1,'A','n','i','m','C','u','r','v','e','N','o','d','e'}), 'S');
                    animationCurveNode.addProperty("");
                    {
                        FBXNode properties("Properties70");
                        {
                            FBXNode p("P");
                            p.addProperty("d|X");
                            p.addProperty("Number");
                            p.addProperty("");
                            p.addProperty("A");
                            p.addProperty((double)x);
                            properties.addChild(p);
                        }
                        {
                            FBXNode p("P");
                            p.addProperty("d|Y");
                            p.addProperty("Number");
                            p.addProperty("");
                            p.addProperty("A");
                            p.addProperty((double)y);
                            properties.addChild(p);
                        }
                        {
                            FBXNode p("P");
                            p.addProperty("d|Z");
                            p.addProperty("Number");
                            p.addProperty("");
                            p.addProperty("A");
                            p.addProperty((double)z);
                            properties.addChild(p);
                        }
                        properties.addChild(FBXNode());
                        animationCurveNode.addChild(properties);
                    }
                    animationCurveNode.addChild(FBXNode());
                    animationCurveNodes.push_back(animationCurveNode);
                    
                    {
                        FBXNode p("C");
                        p.addProperty("OO");
                        p.addProperty(animationCurveNodeId);
                        p.addProperty(animationLayerId);
                        connections.addChild(p);
                    }
                    {
                        FBXNode p("C");
                        p.addProperty("OP");
                        p.addProperty(animationCurveNodeId);
                        p.addProperty(limbNodeIds[1 + jointIndex]);
                        p.addProperty("Lcl Translation");
                        connections.addChild(p);
                    }
                    {
                        FBXNode p("C");
                        p.addProperty("OP");
                        p.addProperty(animationCurveIds[0]);
                        p.addProperty(animationCurveNodeId);
                        p.addProperty("d|X");
                        connections.addChild(p);
                    }
                    {
                        FBXNode p("C");
                        p.addProperty("OP");
                        p.addProperty(animationCurveIds[1]);
                        p.addProperty(animationCurveNodeId);
                        p.addProperty("d|Y");
                        connections.addChild(p);
                    }
                    {
                        FBXNode p("C");
                        p.addProperty("OP");
                        p.addProperty(animationCurveIds[2]);
                        p.addProperty(animationCurveNodeId);
                        p.addProperty("d|Z");
                        connections.addChild(p);
                    }
                }
                
                for (int curveIndex = 0; curveIndex < 3; ++curveIndex)
                {
                    FBXNode animationCurve("AnimationCurve");
                    animationCurve.addProperty(animationCurveIds[curveIndex]);
                    animationCurve.addProperty(std::vector<uint8_t>({'C','u','r','v','e',(uint8_t)('1'+curveIndex),0,1,'A','n','i','m','C','u','r','v','e'}), 'S');
                    animationCurve.addProperty("");
                    animationCurve.addPropertyNode("Default", (double)0.000000);
                    animationCurve.addPropertyNode("KeyVer", (int32_t)4008);
                    animationCurve.addPropertyNode("KeyTime", ktimes);
                    animationCurve.addPropertyNode("KeyValueFloat", values[curveIndex]);
                    animationCurve.addPropertyNode("KeyAttrFlags", std::vector<int>(1, 24836));
                    animationCurve.addPropertyNode("KeyAttrDataFloat", std::vector<float>(4, 0.000000));
                    animationCurve.addPropertyNode("KeyAttrRefCount", std::vector<int32_t>(1, ktimes.size()));
                    animationCurve.addChild(FBXNode());
                    animationCurves.push_back(animationCurve);
                }
            }
            
            for (const auto &jointIndex: rotatedJoints) {
                int64_t animationCurveIds[3];
                std::vector<int64_t> ktimes;
                std::vector<float> values[3];
                for (int curveIndex = 0; curveIndex < 3; ++curveIndex) {
                    animationCurveIds[curveIndex] = m_next64Id++;
                }
                double timePoint = 0;
                for (int frame = 0; frame < (int)motion.second.size(); frame++) {
                    const auto &keyframe = motion.second[frame];
                    const auto &rotation = keyframe.second.nodes()[jointIndex].rotation;
                    double pitch = 0;
                    double yaw = 0;
                    double roll = 0;
                    quaternionToFbxEulerAngles(rotation, &pitch, &yaw, &roll);

                    values[0].push_back(pitch);
                    values[1].push_back(yaw);
                    values[2].push_back(roll);
                    ktimes.push_back(secondsToKtime(timePoint));
                    timePoint += keyframe.first;
                    
                    FBXNode animationCurveNode("AnimationCurveNode");
                    int64_t animationCurveNodeId = m_next64Id++;
                    animationCurveNode.addProperty(animationCurveNodeId);
                    animationCurveNode.addProperty(std::vector<uint8_t>({'R',0,1,'A','n','i','m','C','u','r','v','e','N','o','d','e'}), 'S');
                    animationCurveNode.addProperty("");
                    {
                        FBXNode properties("Properties70");
                        {
                            FBXNode p("P");
                            p.addProperty("d|X");
                            p.addProperty("Number");
                            p.addProperty("");
                            p.addProperty("A");
                            p.addProperty((double)pitch);
                            properties.addChild(p);
                        }
                        {
                            FBXNode p("P");
                            p.addProperty("d|Y");
                            p.addProperty("Number");
                            p.addProperty("");
                            p.addProperty("A");
                            p.addProperty((double)yaw);
                            properties.addChild(p);
                        }
                        {
                            FBXNode p("P");
                            p.addProperty("d|Z");
                            p.addProperty("Number");
                            p.addProperty("");
                            p.addProperty("A");
                            p.addProperty((double)roll);
                            properties.addChild(p);
                        }
                        properties.addChild(FBXNode());
                        animationCurveNode.addChild(properties);
                    }
                    animationCurveNode.addChild(FBXNode());
                    animationCurveNodes.push_back(animationCurveNode);
                    
                    {
                        FBXNode p("C");
                        p.addProperty("OO");
                        p.addProperty(animationCurveNodeId);
                        p.addProperty(animationLayerId);
                        connections.addChild(p);
                    }
                    {
                        FBXNode p("C");
                        p.addProperty("OP");
                        p.addProperty(animationCurveNodeId);
                        p.addProperty(limbNodeIds[1 + jointIndex]);
                        p.addProperty("Lcl Rotation");
                        connections.addChild(p);
                    }
                    {
                        FBXNode p("C");
                        p.addProperty("OP");
                        p.addProperty(animationCurveIds[0]);
                        p.addProperty(animationCurveNodeId);
                        p.addProperty("d|X");
                        connections.addChild(p);
                    }
                    {
                        FBXNode p("C");
                        p.addProperty("OP");
                        p.addProperty(animationCurveIds[1]);
                        p.addProperty(animationCurveNodeId);
                        p.addProperty("d|Y");
                        connections.addChild(p);
                    }
                    {
                        FBXNode p("C");
                        p.addProperty("OP");
                        p.addProperty(animationCurveIds[2]);
                        p.addProperty(animationCurveNodeId);
                        p.addProperty("d|Z");
                        connections.addChild(p);
                    }
                }
                for (int curveIndex = 0; curveIndex < 3; ++curveIndex)
                {
                    FBXNode animationCurve("AnimationCurve");
                    animationCurve.addProperty(animationCurveIds[curveIndex]);
                    animationCurve.addProperty(std::vector<uint8_t>({'C','u','r','v','e',(uint8_t)('1'+curveIndex),0,1,'A','n','i','m','C','u','r','v','e'}), 'S');
                    animationCurve.addProperty("");
                    animationCurve.addPropertyNode("Default", (double)0.000000);
                    animationCurve.addPropertyNode("KeyVer", (int32_t)4008);
                    animationCurve.addPropertyNode("KeyTime", ktimes);
                    animationCurve.addPropertyNode("KeyValueFloat", values[curveIndex]);
                    animationCurve.addPropertyNode("KeyAttrFlags", std::vector<int>(1, 24836));
                    animationCurve.addPropertyNode("KeyAttrDataFloat", std::vector<float>(4, 0.000000));
                    animationCurve.addPropertyNode("KeyAttrRefCount", std::vector<int32_t>(1, ktimes.size()));
                    animationCurve.addChild(FBXNode());
                    animationCurves.push_back(animationCurve);
                }
            }
        }
        
        animationStackCount = animationStacks.size();
        animationLayerCount = animationLayers.size();
        animationCurveNodeCount = animationCurveNodes.size();
        animationCurveCount = animationCurves.size();
    }

    createDefinitions(deformerCount,
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
    for (size_t i = 0; i < boneNodes.size(); ++i) {
        const auto &boneNode = boneNodes[i];
        {
            FBXNode p("C");
            p.addProperty("OO");
            p.addProperty(deformerIds[1 + i]);
            p.addProperty(skinId);
            connections.addChild(p);
        }
        if (-1 == boneNode.parentIndex) {
            FBXNode p("C");
            p.addProperty("OO");
            p.addProperty(limbNodeIds[1 + i]);
            p.addProperty(armatureId);
            connections.addChild(p);
        } else {
            FBXNode p("C");
            p.addProperty("OO");
            p.addProperty(limbNodeIds[1 + i]);
            p.addProperty(limbNodeIds[1 + boneNode.parentIndex]);
            connections.addChild(p);
        }
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

int64_t FbxFileWriter::to64Id(const QUuid &uuid)
{
    QString uuidString = uuid.toString();
    auto insertResult = m_uuidTo64Map.insert({uuidString, m_uuidTo64Map.size()});
    return insertResult.first->second;
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

// http://bediyap.com/programming/convert-quaternion-to-euler-rotations/
static void threeaxisrot(double r11, double r12, double r21, double r31, double r32, double res[])
{
    res[0] = atan2(r31, r32);
    res[1] = asin(r21);
    res[2] = atan2(r11, r12);
}

void FbxFileWriter::quaternionToFbxEulerAngles(const QQuaternion &q, double *pitch, double *yaw, double *roll)
{
    double radians[3] = {0, 0, 0};
    threeaxisrot(2*(q.x()*q.y() + q.scalar()*q.z()),
        q.scalar()*q.scalar() + q.x()*q.x() - q.y()*q.y() - q.z()*q.z(),
        -2*(q.x()*q.z() - q.scalar()*q.y()),
        2*(q.y()*q.z() + q.scalar()*q.x()),
        q.scalar()*q.scalar() - q.x()*q.x() - q.y()*q.y() + q.z()*q.z(),
        radians);
    *pitch = qRadiansToDegrees(radians[0]);
    *yaw = qRadiansToDegrees(radians[1]);
    *roll = qRadiansToDegrees(radians[2]);
}
