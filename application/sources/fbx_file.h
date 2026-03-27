#ifndef DUST3D_APPLICATION_FBX_FILE_H_
#define DUST3D_APPLICATION_FBX_FILE_H_

#include "bone_structure.h"
#include "fbxdocument.h"
#include <QImage>
#include <QMatrix4x4>
#include <QObject>
#include <QQuaternion>
#include <QString>
#include <dust3d/animation/animation_generator.h>
#include <dust3d/base/matrix4x4.h>
#include <dust3d/base/object.h>
#include <map>
#include <vector>

class FbxFileWriter : public QObject {
    Q_OBJECT
public:
    FbxFileWriter(dust3d::Object& object,
        const QString& filename,
        QImage* textureImage = nullptr,
        QImage* normalImage = nullptr,
        QImage* metalnessImage = nullptr,
        QImage* roughnessImage = nullptr,
        QImage* ambientOcclusionImage = nullptr,
        const RigStructure* rigStructure = nullptr,
        const std::map<std::string, dust3d::Matrix4x4>* inverseBindMatrices = nullptr,
        const std::vector<dust3d::RigAnimationClip>* animationClips = nullptr);
    bool save();

private:
    void createFbxHeader();
    void createCreationTime();
    void createFileId();
    void createCreator();
    void createGlobalSettings();
    void createDocuments();
    void createReferences();
    void createDefinitions(size_t deformerCount,
        size_t textureCount = 0,
        size_t videoCount = 0,
        bool hasAnimtion = false,
        size_t animationStackCount = 0,
        size_t animationLayerCount = 0,
        size_t animationCurveNodeCount = 0,
        size_t animationCurveCount = 0);
    void createTakes();
    std::vector<double> matrixToVector(const QMatrix4x4& matrix);
    int64_t secondsToKtime(double seconds);

    int64_t m_next64Id = 612150000;
    QString m_filename;
    QString m_baseName;
    fbx::FBXDocument m_fbxDocument;
    std::map<QString, int64_t> m_uuidTo64Map;
    static std::vector<double> m_identityMatrix;
};

#endif
