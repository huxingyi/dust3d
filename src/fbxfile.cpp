#include <fbxnode.h>
#include <fbxproperty.h>
#include <QDateTime>
#include "fbxfile.h"
#include "version.h"

using namespace fbx;

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
            p.addProperty((double)1.000000);
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

void FbxFileWriter::createDefinitions()
{
    FBXNode definitions("Definitions");
    definitions.addPropertyNode("Version", (int32_t)100);
    definitions.addPropertyNode("Count", (int32_t)4);
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
        count.addProperty((int32_t)1);
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
    definitions.addChild(FBXNode());
    m_fbxDocument.nodes.push_back(definitions);
}

FbxFileWriter::FbxFileWriter(MeshResultContext &resultContext,
        const QString &filename) :
    m_filename(filename)
{
    createFbxHeader();
    createFileId();
    createCreationTime();
    createCreator();
    createGlobalSettings();
    createDocuments();
    createReferences();
    createDefinitions();
    
    FBXNode geometry("Geometry");
    int64_t geometryId = m_next64Id++;
    geometry.addProperty(geometryId);
    geometry.addProperty(std::vector<uint8_t>({'u','n','a','m','e','d','m','e','s','h',0,1,'G','e','o','m','e','t','r','y'}), 'S');
    geometry.addProperty("Mesh");
    std::vector<double> positions;
    for (const auto &vertex: resultContext.vertices) {
        positions.push_back((double)vertex.position.x());
        positions.push_back((double)vertex.position.y());
        positions.push_back((double)vertex.position.z());
    }
    std::vector<int32_t> indicies;
    for (const auto &triangle: resultContext.triangles) {
        indicies.push_back(triangle.indicies[0]);
        indicies.push_back(triangle.indicies[1]);
        indicies.push_back(triangle.indicies[2] ^ -1);
    }
    FBXNode layerElementNormal("LayerElementNormal");
    layerElementNormal.addProperty((int32_t)0);
    layerElementNormal.addPropertyNode("Version", (int32_t)101);
    layerElementNormal.addPropertyNode("Name", "");
    layerElementNormal.addPropertyNode("MappingInformationType", "ByPolygonVertex");
    layerElementNormal.addPropertyNode("ReferenceInformationType", "Direct");
    std::vector<double> normals;
    const auto &triangleVertexNormals = resultContext.interpolatedTriangleVertexNormals();
    for (decltype(triangleVertexNormals.size()) i = 0; i < triangleVertexNormals.size(); ++i) {
        const auto &n = triangleVertexNormals[i];
        normals.push_back((double)n.x());
        normals.push_back((double)n.y());
        normals.push_back((double)n.z());
    }
    layerElementNormal.addPropertyNode("Normals", normals);
    layerElementNormal.addChild(FBXNode());
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
    geometry.addPropertyNode("PolygonVertexIndex", indicies);
    geometry.addChild(layerElementNormal);
    geometry.addChild(layerElementMaterial);
    geometry.addChild(layer);
    geometry.addChild(FBXNode());
    
    FBXNode model("Model");
    int64_t modelId = m_next64Id++;
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
            p.addProperty((double)100.000000);
            p.addProperty((double)100.000000);
            p.addProperty((double)100.000000);
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
    
    FBXNode objects("Objects");
    objects.addChild(geometry);
    objects.addChild(model);
    objects.addChild(material);
    objects.addChild(FBXNode());
    m_fbxDocument.nodes.push_back(objects);
    
    FBXNode connections("Connections");
    {
        FBXNode p("C");
        p.addProperty("OO");
        p.addProperty(modelId);
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
    m_fbxDocument.print();
    m_fbxDocument.write(m_filename.toStdString());
    return true;
}
