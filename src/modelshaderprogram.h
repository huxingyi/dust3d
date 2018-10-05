#ifndef MODEL_SHADER_PROGRAM_H
#define MODEL_SHADER_PROGRAM_H
#include <QOpenGLShaderProgram>
#include <QString>

class ModelShaderProgram : public QOpenGLShaderProgram
{
public:
    ModelShaderProgram(bool usePBR=true);
    int projectionMatrixLoc();
    int modelMatrixLoc();
    int viewMatrixLoc();
    int lightPosLoc();
    int textureIdLoc();
    int textureEnabledLoc();
    static const QString &loadShaderSource(const QString &name);
private:
    int m_projectionMatrixLoc;
    int m_modelMatrixLoc;
    int m_viewMatrixLoc;
    int m_lightPosLoc;
    int m_textureIdLoc;
    int m_textureEnabledLoc;
};

#endif
