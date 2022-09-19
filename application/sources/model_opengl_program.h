#ifndef DUST3D_APPLICATION_MODEL_OPENGL_PROGRAM_H_
#define DUST3D_APPLICATION_MODEL_OPENGL_PROGRAM_H_

#include <QOpenGLShaderProgram>

class ModelOpenGLProgram: public QOpenGLShaderProgram
{
public:
    void load(bool isCoreProfile=false);

private:
    bool m_isLoaded = false;
    bool m_isCoreProfile = false;
};

#endif
