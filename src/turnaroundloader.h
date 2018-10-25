#ifndef DUST3D_TURNAROUND_LOADER_H
#define DUST3D_TURNAROUND_LOADER_H
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
    QImage *m_resultImage;
    QImage m_inputImage;
    QString m_filename;
    QSize m_viewSize;
};

#endif
