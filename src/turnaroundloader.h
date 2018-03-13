#ifndef TURNAROUND_LOADER_H
#define TURNAROUND_LOADER_H
#include <QObject>
#include <QString>
#include <QSize>
#include <QImage>

class TurnaroundLoader : public QObject
{
    Q_OBJECT
public:
    TurnaroundLoader(const QString &filename, QSize viewSize);
    ~TurnaroundLoader();
    QImage *takeResultImage();
signals:
    void finished();
public slots:
    void process();
private:
    QImage *m_resultImage;
    QString m_filename;
    QSize m_viewSize;
};

#endif
