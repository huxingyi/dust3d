#ifndef DUST3D_APPLICATION_RIG_GENERATOR_WORKER_H_
#define DUST3D_APPLICATION_RIG_GENERATOR_WORKER_H_

#include <QObject>
#include <dust3d/base/snapshot.h>
#include <dust3d/rig/rig_generator.h>
#include <memory>
#include "bone_structure.h"

class RigGeneratorWorker : public QObject {
    Q_OBJECT
public:
    void setParameters(std::unique_ptr<dust3d::Snapshot> snapshot, const RigStructure& templateRig)
    {
        m_snapshot = std::move(snapshot);
        m_templateRig = templateRig;
    }

    const RigStructure& getActualRig() const { return m_actualRig; }
    bool isSuccessful() const { return m_successful; }

signals:
    void finished();

public slots:
    void process()
    {
        dust3d::RigGenerator generator;
        dust3d::RigStructure templateRig = m_templateRig.toRigStructure();
        dust3d::RigStructure actualRig;

        m_successful = generator.generateRig(m_snapshot.get(), templateRig, actualRig);
        if (m_successful) {
            m_actualRig = RigStructure(actualRig);
        }

        emit finished();
    }

private:
    std::unique_ptr<dust3d::Snapshot> m_snapshot;
    RigStructure m_templateRig;
    RigStructure m_actualRig;
    bool m_successful = false;
};

#endif
