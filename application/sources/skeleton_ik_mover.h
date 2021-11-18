#ifndef DUST3D_APPLICATION_SKELETON_IK_MOVER_H_
#define DUST3D_APPLICATION_SKELETON_IK_MOVER_H_

#include <QObject>
#include <vector>
#include <QVector3D>
#include <dust3d/base/uuid.h>

struct SkeletonIkNode
{
    dust3d::Uuid id;
    QVector3D position;
    QVector3D newPosition;
};

class SkeletonIkMover : public QObject
{
    Q_OBJECT
public:
    SkeletonIkMover();
    ~SkeletonIkMover();
    void setTarget(const QVector3D &target);
    void setUpdateVersion(unsigned long long version);
    unsigned long long getUpdateVersion();
    void addNode(dust3d::Uuid id, QVector3D position);
    const std::vector<SkeletonIkNode> &ikNodes();
signals:
    void finished();
public slots:
    void process();
private:
    void resolve();
private:
    unsigned long long m_updateVersion = 0;
    std::vector<SkeletonIkNode> m_ikNodes;
    QVector3D m_target;
};

#endif
