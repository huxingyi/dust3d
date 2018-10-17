#ifndef FBX_FILE_H
#define FBX_FILE_H
#include <fbxdocument.h>
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
    
    QString m_filename;
    fbx::FBXDocument m_fbxDocument;
};

#endif

