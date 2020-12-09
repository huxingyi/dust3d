#ifndef DUST3D_VERTEBRATA_MOVE_MOTION_PARAMETERS_WIDGET_H
#define DUST3D_VERTEBRATA_MOVE_MOTION_PARAMETERS_WIDGET_H
#include <QWidget>
#include <map>
#include <QString>
#include "vertebratamovemotionbuilder.h"

class VertebrataMoveMotionParametersWidget : public QWidget
{
    Q_OBJECT
signals:
    void parametersChanged();
public:
    VertebrataMoveMotionParametersWidget(const std::map<QString, QString> &parameters=std::map<QString, QString>());
    
    const std::map<QString, QString> &getParameters() const
    {
        return m_parameters;
    }
    
    static std::map<QString, QString> fromVertebrataMoveMotionParameters(const VertebrataMoveMotionBuilder::Parameters &from);
    static VertebrataMoveMotionBuilder::Parameters toVertebrataMoveMotionParameters(const std::map<QString, QString> &parameters);
private:
    std::map<QString, QString> m_parameters;
    VertebrataMoveMotionBuilder::Parameters m_vertebrataMotionParameters;
};

#endif
