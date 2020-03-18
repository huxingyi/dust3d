#ifndef DUST3D_DDS_FILE_H
#define DUST3D_DDS_FILE_H
#include <QString>
#include <QOpenGLTexture>

class DdsFileReader
{
public:
    DdsFileReader(const QString &filename);
    QOpenGLTexture *createOpenGLTexture();
private:
    QString m_filename;
};

#endif