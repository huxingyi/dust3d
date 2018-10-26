#ifndef DUST3D_FBX_FILE_H
#define DUST3D_FBX_FILE_H
#include <fbxdocument.h>
#include <map>
#include <QString>
#include <QMatrix4x4>
#include "outcome.h"
#include "document.h"

class FbxFileWriter : public QObject
{
    Q_OBJECT
public:
    FbxFileWriter(Outcome &outcome,
        const std::vector<RiggerBone> *resultRigBones,
        const std::map<int, RiggerVertexWeights> *resultRigWeights,
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
    void createDefinitions(size_t deformerCount);
    void createTakes();
    std::vector<double> matrixToVector(const QMatrix4x4 &matrix);
    
    int64_t to64Id(const QUuid &uuid);
    int64_t m_next64Id = 612150000;
    QString m_filename;
    fbx::FBXDocument m_fbxDocument;
    std::map<QString, int64_t> m_uuidTo64Map;
    static std::vector<double> m_identityMatrix;
};

#endif

