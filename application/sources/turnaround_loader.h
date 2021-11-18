#ifndef DUST3D_APPLICATION_TURNAROUND_LOADER_H_
#define DUST3D_APPLICATION_TURNAROUND_LOADER_H_

#include <QObject>
#include <QString>
#include <QSize>
#include <QImage>

class TurnaroundLoader : public QObject
{
    Q_OBJECT
public:
    TurnaroundLoader(const QString &filename, QSize viewSize);
    TurnaroundLoader(const QImage &image, QSize viewSize);
    ~TurnaroundLoader();
    QImage *takeResultImage();
signals:
    void finished();
public slots:
    void process();
private:
    QImage *m_resultImage = nullptr;
    QImage m_inputImage;
    QString m_filename;
    QSize m_viewSize;
};

#endif
