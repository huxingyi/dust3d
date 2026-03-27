#include "export_animation_worker.h"
#include <QDebug>

void ExportAnimationWorker::process()
{
    m_animationClips.clear();
    m_inverseBindMatrices.clear();
    m_successful = false;

    dust3d::RigStructure baseRig = m_rigStructure.toRigStructure();

    dust3d::RigGenerator rigGenerator;
    if (!rigGenerator.computeBoneInverseBindMatrices(baseRig, m_inverseBindMatrices)) {
        qWarning() << "Export animation: failed to compute inverse bind matrices:"
                   << QString::fromStdString(rigGenerator.getErrorMessage());
        emit finished();
        return;
    }

    int total = (int)m_animations.size();
    int current = 0;

    for (const auto& animation : m_animations) {
        emit progress(current, total);
        ++current;

        dust3d::AnimationParams params;
        params.values = animation.params;

        dust3d::RigAnimationClip clip;
        clip.name = animation.name.toStdString();

        if (!dust3d::AnimationGenerator::generate(baseRig, m_inverseBindMatrices, clip,
                animation.type.toStdString(), m_frameCount, m_durationSeconds, params)) {
            qWarning() << "Export animation: generate failed for animation"
                       << animation.name;
            // Push an empty clip so indices align with input animations list
            clip.frames.clear();
        }

        m_animationClips.push_back(std::move(clip));
    }

    emit progress(total, total);

    m_successful = true;
    emit finished();
}
