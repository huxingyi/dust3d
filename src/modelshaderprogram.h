#ifndef MODEL_SHADER_PROGRAM_H
#define MODEL_SHADER_PROGRAM_H
#include <QOpenGLShaderProgram>

class ModelShaderProgram : public QOpenGLShaderProgram
{
public:
    ModelShaderProgram();
    int projMatrixLoc();
    int mvMatrixLoc();
    int normalMatrixLoc();
    int lightPosLoc();
    int textureIdLoc();
    int textureEnabledLoc();
private:
    int m_projMatrixLoc;
    int m_mvMatrixLoc;
    int m_normalMatrixLoc;
    int m_lightPosLoc;
    int m_textureIdLoc;
    int m_textureEnabledLoc;
};

#endif
