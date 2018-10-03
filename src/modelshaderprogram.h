#ifndef MODEL_SHADER_PROGRAM_H
#define MODEL_SHADER_PROGRAM_H
#include <QOpenGLShaderProgram>
#include <QString>

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
    static const QString &loadShaderSource(const QString &name);
private:
    int m_projMatrixLoc;
    int m_mvMatrixLoc;
    int m_normalMatrixLoc;
    int m_lightPosLoc;
    int m_textureIdLoc;
    int m_textureEnabledLoc;
};

#endif
