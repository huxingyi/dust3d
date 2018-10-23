#ifndef TAB_WIDGET_H
#define TAB_WIDGET_H
#include <QWidget>
#include <QString>
#include <vector>
#include <QPushButton>

class TabWidget : public QWidget
{
    Q_OBJECT
signals:
    void currentIndexChanged(int index);
    
public:
    TabWidget(const std::vector<QString> &items, QWidget *parent=nullptr);
    void setCurrentIndex(int index);
    int currentIndex();

private slots:
    void itemClicked(int index);
    
private:
    void updateButtonLook(int index, bool checked);

    std::vector<QPushButton *> m_buttons;
    int m_currentIndex = -1;
};

#endif
