#ifndef MESH_RESULT_POST_PROCESSOR_H
#define MESH_RESULT_POST_PROCESSOR_H
#include <QObject>
#include "meshresultcontext.h"
#include "jointnodetree.h"

class MeshResultPostProcessor : public QObject
{
    Q_OBJECT
public:
    MeshResultPostProcessor(const MeshResultContext &meshResultContext);
    ~MeshResultPostProcessor();
    MeshResultContext *takePostProcessedResultContext();
    JointNodeTree *takeJointNodeTree();
signals:
    void finished();
public slots:
    void process();
private:
    MeshResultContext *m_meshResultContext = nullptr;
    JointNodeTree *m_jointNodeTree = nullptr;
};

#endif
