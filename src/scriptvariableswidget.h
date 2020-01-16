#ifndef DUST3D_SCRIPT_VARIABLES_WIDGET_H
#define DUST3D_SCRIPT_VARIABLES_WIDGET_H
#include <QWidget>
#include <QString>
#include <map>
#include <QScrollArea>
#include "document.h"

class ScriptVariablesWidget : public QScrollArea
{
    Q_OBJECT
signals:
    void updateVariableValue(const QString &name, const QString &value);
public:
    ScriptVariablesWidget(const Document *document,
        QWidget *parent=nullptr);
public slots:
    void reload();
protected:
    QSize sizeHint() const override;
private:
    const Document *m_document = nullptr;
};

#endif
