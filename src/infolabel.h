#ifndef INFO_LABEL_H
#define INFO_LABEL_H
#include <QLabel>
#include <QIcon>
#include <QWidget>

class InfoLabel : public QWidget
{
public:
    InfoLabel(const QString &text=QString(), QWidget *parent=nullptr);
    void setText(const QString &text);
private:
    QLabel *m_label = nullptr;
    QLabel *m_icon = nullptr;
};

#endif
