#ifndef DUST3D_VERTEBRATA_MOTION_PARAMETERS_WIDGET_H
#define DUST3D_VERTEBRATA_MOTION_PARAMETERS_WIDGET_H
#include <QWidget>
#include <map>
#include <QString>
#include "vertebratamotion.h"

class VertebrataMotionParametersWidget : public QWidget
{
    Q_OBJECT
signals:
    void parametersChanged();
public:
    VertebrataMotionParametersWidget(const std::map<QString, QString> &parameters=std::map<QString, QString>());
    
    const std::map<QString, QString> &getParameters() const
    {
        return m_parameters;
    }
    
    static std::map<QString, QString> fromVertebrataMotionParameters(const VertebrataMotion::Parameters &from);
    static VertebrataMotion::Parameters toVertebrataMotionParameters(const std::map<QString, QString> &parameters);
private:
    std::map<QString, QString> m_parameters;
    VertebrataMotion::Parameters m_vertebrataMotionParameters;
};

#endif
