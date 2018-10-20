#ifndef FBX_FILE_H
#define FBX_FILE_H
#include <fbxdocument.h>
#include <map>
#include <QString>
#include "meshresultcontext.h"
#include "skeletondocument.h"

class FbxFileWriter : public QObject
{
    Q_OBJECT
public:
    FbxFileWriter(MeshResultContext &resultContext,
        const QString &filename);
    bool save();

private:
    void createFbxHeader();
    void createCreationTime();
    void createFileId();
    void createCreator();
    void createGlobalSettings();
    void createDocuments();
    void createReferences();
    void createDefinitions();
    void createTakes();
    
    int64_t to64Id(const QUuid &uuid);
    int64_t m_next64Id = 612150000;
    QString m_filename;
    fbx::FBXDocument m_fbxDocument;
    std::map<QString, int64_t> m_uuidTo64Map;
};

#endif

