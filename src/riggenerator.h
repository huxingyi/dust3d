#ifndef RIG_GENERATOR_H
#define RIG_GENERATOR_H
#include <QObject>
#include <QThread>
#include <QDebug>
#include "meshresultcontext.h"
#include "meshloader.h"
#include "autorigger.h"

class RigGenerator : public QObject
{
    Q_OBJECT
public:
    RigGenerator(const MeshResultContext &meshResultContext);
    ~RigGenerator();
    MeshLoader *takeResultMesh();
    std::vector<AutoRiggerBone> *takeResultBones();
    std::map<int, AutoRiggerVertexWeights> *takeResultWeights();
    const std::vector<QString> &missingMarkNames();
    const std::vector<QString> &errorMarkNames();
    MeshResultContext *takeMeshResultContext();
signals:
    void finished();
public slots:
    void process();
private:
    MeshResultContext *m_meshResultContext = nullptr;
    MeshLoader *m_resultMesh = nullptr;
    AutoRigger *m_autoRigger = nullptr;
    std::vector<AutoRiggerBone> *m_resultBones = nullptr;
    std::map<int, AutoRiggerVertexWeights> *m_resultWeights = nullptr;
    std::vector<QString> m_missingMarkNames;
    std::vector<QString> m_errorMarkNames;
};

#endif
