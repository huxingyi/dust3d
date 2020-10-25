#ifndef DUST3D_MOTION_PREVIEW_GENERATOR_H
#define DUST3D_MOTION_PREVIEW_GENERATOR_H
#include <QObject>
#include "vertebratamotion.h"
#include "rigger.h"

class MotionPreviewGenerator: public QObject
{
    Q_OBJECT
public:
    MotionPreviewGenerator(const std::vector<RiggerBone> &bones,
            const VertebrataMotion::Parameters &parameters) :
        m_bones(bones),
        m_parameters(parameters)
    {
    }
    
    ~MotionPreviewGenerator()
    {
        delete m_vertebrataMotion;
    }
    
    void generate();
    
    const std::vector<VertebrataMotion::FrameMesh> &frames()
    {
        return m_vertebrataMotion->frames();
    }
    
signals:
    void finished();
public slots:
    void process();
    
private:
    VertebrataMotion::Parameters m_parameters;
    std::vector<RiggerBone> m_bones;
    VertebrataMotion *m_vertebrataMotion = nullptr;
};

#endif
