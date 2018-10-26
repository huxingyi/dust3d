#ifndef DUST3D_MESH_RESULT_POST_PROCESSOR_H
#define DUST3D_MESH_RESULT_POST_PROCESSOR_H
#include <QObject>
#include "outcome.h"

class MeshResultPostProcessor : public QObject
{
    Q_OBJECT
public:
    MeshResultPostProcessor(const Outcome &outcome);
    ~MeshResultPostProcessor();
    Outcome *takePostProcessedOutcome();
    void poseProcess();
signals:
    void finished();
public slots:
    void process();
private:
    Outcome *m_outcome = nullptr;
};

#endif
