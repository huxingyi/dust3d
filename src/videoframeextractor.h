#ifndef VIDEO_FRAME_EXTRACTOR_H
#define VIDEO_FRAME_EXTRACTOR_H
#include <QObject>
#include <QImage>
#include <QTemporaryFile>

struct VideoFrame
{
    QImage image;
    QImage thumbnail;
    qint64 position;
    float durationSeconds = 0;
};

class VideoFrameExtractor : public QObject
{
    Q_OBJECT
public:
    VideoFrameExtractor(const QString &fileName, const QString &realPath, QTemporaryFile *fileHandle, float thumbnailHeight, int maxFrames=(10 * 60));
    ~VideoFrameExtractor();
    const QString &fileName();
    const QString &realPath();
    QTemporaryFile *fileHandle();
    void extract();
    std::vector<VideoFrame> *takeResultFrames();
signals:
    void finished();
public slots:
    void process();
private:
    void release();
private:
    QString m_fileName;
    QString m_realPath;
    QTemporaryFile *m_fileHandle = nullptr;
    std::vector<VideoFrame> *m_resultFrames = nullptr;
    int m_maxFrames;
    float m_thumbnailHeight = 0;
};

#endif
