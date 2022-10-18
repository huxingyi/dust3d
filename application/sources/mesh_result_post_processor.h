#ifndef DUST3D_APPLICATION_MESH_RESULT_POST_PROCESSOR_H_
#define DUST3D_APPLICATION_MESH_RESULT_POST_PROCESSOR_H_

#include <QObject>
#include <dust3d/base/object.h>

class MeshResultPostProcessor : public QObject {
    Q_OBJECT
public:
    MeshResultPostProcessor(const dust3d::Object& object);
    ~MeshResultPostProcessor();
    dust3d::Object* takePostProcessedObject();
    void poseProcess();
signals:
    void finished();
public slots:
    void process();

private:
    dust3d::Object* m_object = nullptr;
};

#endif
