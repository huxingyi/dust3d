#ifndef DUST3D_APPLICATION_DDS_FILE_H_
#define DUST3D_APPLICATION_DDS_FILE_H_

#include <QOpenGLTexture>
#include <QString>
#include <memory>

class DdsFileReader {
public:
    DdsFileReader(const QString& filename);
    QOpenGLTexture* createOpenGLTexture();
    std::unique_ptr<std::vector<std::unique_ptr<QOpenGLTexture>>> createOpenGLTextures();

private:
    QString m_filename;
};

#endif
