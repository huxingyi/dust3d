#include <fbxnode.h>
#include <fbxproperty.h>
#include <QDate>
#include "fbxfile.h"
#include "version.h"

using namespace fbx;

FbxFileWriter::FbxFileWriter(MeshResultContext &resultContext,
        const QString &filename) :
    m_filename(filename)
{
    createFbxHeader();
}

void FbxFileWriter::createFbxHeader()
{
    FBXNode headerExtension("FBXHeaderExtension");
    headerExtension.addPropertyNode("FBXHeaderVersion", (int32_t) 1003);
    headerExtension.addPropertyNode("FBXVersion", (int32_t) m_fbxDocument.getVersion());
    headerExtension.addPropertyNode("EncryptionType", (int32_t) 0);
    {
        auto currentDate = QDate::currentDate();
        
        FBXNode creationTimeStamp("CreationTimeStamp");
        creationTimeStamp.addPropertyNode("Version", (int32_t) 1000);
        creationTimeStamp.addPropertyNode("Year", (int32_t) currentDate.year());
        creationTimeStamp.addPropertyNode("Month", (int32_t) currentDate.month());
        creationTimeStamp.addPropertyNode("Day", (int32_t) currentDate.day());
        creationTimeStamp.addPropertyNode("Hour", (int32_t) 0);
        creationTimeStamp.addPropertyNode("Minute", (int32_t) 0);
        creationTimeStamp.addPropertyNode("Second", (int32_t) 0);
        creationTimeStamp.addPropertyNode("Millisecond", (int32_t) 0);
        headerExtension.addChild(creationTimeStamp);
    }
    headerExtension.addPropertyNode("Creator", APP_NAME APP_HUMAN_VER);
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
                p.addProperty("Blender Foundation");
                properties.addChild(p);
            }
            {
                FBXNode p("P");
                p.addProperty("LastSaved|ApplicationName");
                p.addProperty("KString");
                p.addProperty("");
                p.addProperty("");
                p.addProperty("Blender (stable FBX IO)");
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
            sceneInfo.addChild(properties);
        }
        headerExtension.addChild(sceneInfo);
    }
    
    m_fbxDocument.nodes.push_back(headerExtension);
}

bool FbxFileWriter::save()
{
    m_fbxDocument.write(m_filename.toStdString());
    return true;
}
