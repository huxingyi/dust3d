#ifndef DUST3D_APPLICATION_INT_NUMBER_WIDGET_H_
#define DUST3D_APPLICATION_INT_NUMBER_WIDGET_H_

#include <QToolButton>

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QSlider)

class IntNumberWidget : public QWidget {
    Q_OBJECT
public:
    explicit IntNumberWidget(QWidget* parent = nullptr, bool singleLine = true);
    void setRange(int min, int max);
    int value() const;
    void setItemName(const QString& name);

public slots:
    void increaseValue();
    void descreaseValue();
    void setValue(int value);

signals:
    void valueChanged(int value);

private:
    void updateValueLabel(int value);

private:
    QLabel* m_label = nullptr;
    QSlider* m_slider = nullptr;
    QString m_itemName;
};

#endif
