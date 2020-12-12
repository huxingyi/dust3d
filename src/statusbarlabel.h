#ifndef STATUS_BAR_LABEL_H
#define STATUS_BAR_LABEL_H
#include <QWidget>
#include <QLabel>
#include <QEvent>
#include <QHoverEvent>
#include <QString>
#include <QMouseEvent>

class StatusBarLabel: public QWidget
{
    Q_OBJECT
signals:
    void clicked();
public:
    explicit StatusBarLabel(QWidget *parent=0);
    
    void updateText(const QString &text);
    void setSelected(bool selected);
    
protected:
    void mousePressEvent(QMouseEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    bool event(QEvent *event);
    void hoverEnter(QHoverEvent *event);
    void hoverLeave(QHoverEvent *event);
    void hoverMove(QHoverEvent *event);
    
private:
    QLabel *m_label = nullptr;
    bool m_selected = false;
    bool m_hovered = false;
    
    void updateBackground();
};

#endif
