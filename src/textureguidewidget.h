#ifndef TEXTURE_GUIDE_WIDGET_H
#define TEXTURE_GUIDE_WIDGET_H
#include <QWidget>
#include <QImage>
#include <QLabel>
#include <QPushButton>

class TextureGuideWidget : public QWidget
{
    Q_OBJECT
signals:
    void regenerate();
public:
    TextureGuideWidget();
    void updateGuideImage(const QImage &image);
protected:
    void resizeEvent(QResizeEvent *event);
private:
    void resizeImage();
    void initAwesomeButton(QPushButton *button);
private:
    QLabel *m_previewLabel;
    QImage m_image;
};

#endif
