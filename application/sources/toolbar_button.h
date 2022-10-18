#ifndef DUST3D_APPLICATION_TOOLBAR_BUTTON_H_
#define DUST3D_APPLICATION_TOOLBAR_BUTTON_H_

#include <QPushButton>
#include <QString>

class QSvgWidget;

class ToolbarButton : public QPushButton {
    Q_OBJECT
public:
    ToolbarButton(const QString& filename = QString(), QWidget* parent = nullptr);
    void setIcon(const QString& filename);

private:
    QSvgWidget* m_iconWidget = nullptr;
};

#endif
