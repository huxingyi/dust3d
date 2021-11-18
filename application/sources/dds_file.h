#ifndef DUST3D_APPLICATION_DDS_FILE_H_
#define DUST3D_APPLICATION_DDS_FILE_H_

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
