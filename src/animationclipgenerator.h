#ifndef ANIMATION_CLIP_GENERATOR_H
#define ANIMATION_CLIP_GENERATOR_H
#include <QObject>
#include <map>
#include <QString>
#include "meshresultcontext.h"
#include "meshloader.h"

class AnimationClipGenerator : public QObject
{
    Q_OBJECT
signals:
    void finished();
public slots:
    void process();
public:
    AnimationClipGenerator(const MeshResultContext &resultContext,
        const QString &motionName, const std::map<QString, QString> &parameters);
    ~AnimationClipGenerator();
    std::vector<std::pair<int, MeshLoader *>> takeFrameMeshes();
    void generate();
private:
    MeshResultContext m_resultContext;
    QString m_motionName;
    std::map<QString, QString> m_parameters;
    std::vector<std::pair<int, MeshLoader *>> m_frameMeshes;
};

#endif
