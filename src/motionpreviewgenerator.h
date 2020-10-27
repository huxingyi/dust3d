#ifndef DUST3D_MOTION_PREVIEW_GENERATOR_H
#define DUST3D_MOTION_PREVIEW_GENERATOR_H
#include <QObject>
#include "vertebratamotion.h"
#include "rigger.h"
#include "outcome.h"

class MotionPreviewGenerator: public QObject
{
    Q_OBJECT
public:
    MotionPreviewGenerator(const std::vector<RiggerBone> &bones,
            const std::map<int, RiggerVertexWeights> &rigWeights,
            const Outcome &outcome,
            const VertebrataMotion::Parameters &parameters) :
        m_bones(new std::vector<RiggerBone>(bones)),
        m_rigWeights(new std::map<int, RiggerVertexWeights>(rigWeights)),
        m_outcome(new Outcome(outcome)),
        m_parameters(parameters)
    {
    }
    
    ~MotionPreviewGenerator()
    {
        delete m_vertebrataMotion;
        delete m_bones;
        delete m_rigWeights;
        delete m_outcome;
    }
    
    void generate();
    
    const std::vector<VertebrataMotion::FrameMesh> &frames()
    {
        if (m_previewSkinnedMesh)
            return m_skinnedFrames;
        return m_vertebrataMotion->frames();
    }
    
    std::vector<RiggerBone> *takeBones()
    {
        std::vector<RiggerBone> *bones = m_bones;
        m_bones = nullptr;
        return bones;
    }
    
    std::map<int, RiggerVertexWeights> *takeRigWeights()
    {
        std::map<int, RiggerVertexWeights> *rigWeights = m_rigWeights;
        m_rigWeights = nullptr;
        return rigWeights;
    }
    
    Outcome *takeOutcome()
    {
        Outcome *outcome = m_outcome;
        m_outcome = nullptr;
        return outcome;
    }
    
signals:
    void finished();
public slots:
    void process();
    
private:
    VertebrataMotion::Parameters m_parameters;
    std::vector<RiggerBone> *m_bones = nullptr;
    std::map<int, RiggerVertexWeights> *m_rigWeights = nullptr;
    Outcome *m_outcome = nullptr;
    VertebrataMotion *m_vertebrataMotion = nullptr;
    bool m_previewSkinnedMesh = true;
    std::vector<VertebrataMotion::FrameMesh> m_skinnedFrames;
    double m_groundY = 0;
    
    void makeSkinnedMesh();
};

#endif
