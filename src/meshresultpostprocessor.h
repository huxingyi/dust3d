#ifndef DUST3D_MESH_RESULT_POST_PROCESSOR_H
#define DUST3D_MESH_RESULT_POST_PROCESSOR_H
#include <QObject>
#include "object.h"

class MeshResultPostProcessor : public QObject
{
    Q_OBJECT
public:
    MeshResultPostProcessor(const Object &object);
    ~MeshResultPostProcessor();
    Object *takePostProcessedObject();
    void poseProcess();
signals:
    void finished();
public slots:
    void process();
private:
    Object *m_object = nullptr;
};

#endif
