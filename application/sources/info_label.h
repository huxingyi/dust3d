#ifndef DUST3D_APPLICATION_INFO_LABEL_H_
#define DUST3D_APPLICATION_INFO_LABEL_H_

#include <QIcon>
#include <QLabel>
#include <QWidget>

class InfoLabel : public QWidget {
public:
    InfoLabel(const QString& text = QString(), QWidget* parent = nullptr);
    void setText(const QString& text);

private:
    QLabel* m_label = nullptr;
    QLabel* m_icon = nullptr;
};

#endif
