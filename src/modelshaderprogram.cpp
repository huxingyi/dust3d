#include <QSurfaceFormat>
#include "modelshaderprogram.h"

static const char *vertexShaderSourceCore =
    "#version 150\n"
    "in vec4 vertex;\n"
    "in vec3 normal;\n"
    "in vec3 color;\n"
    "in vec2 texCoord;\n"
    "out vec3 vert;\n"
    "out vec3 vertNormal;\n"
    "out vec3 vertColor;\n"
    "out vec2 vertTexCoord;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "uniform mat3 normalMatrix;\n"
    "void main() {\n"
    "   vert = vertex.xyz;\n"
    "   vertNormal = normalMatrix * normal;\n"
    "   vertColor = color;\n"
    "   vertTexCoord = texCoord;\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "}\n";

static const char *vertexShaderSource =
    "attribute vec4 vertex;\n"
    "attribute vec3 normal;\n"
    "attribute vec3 color;\n"
    "attribute vec2 texCoord;\n"
    "varying vec3 vert;\n"
    "varying vec3 vertNormal;\n"
    "varying vec3 vertColor;\n"
    "varying vec2 vertTexCoord;\n"
    "uniform mat4 projMatrix;\n"
    "uniform mat4 mvMatrix;\n"
    "uniform mat3 normalMatrix;\n"
    "void main() {\n"
    "   vert = vertex.xyz;\n"
    "   vertNormal = normalMatrix * normal;\n"
    "   vertColor = color;\n"
    "   vertTexCoord = texCoord;\n"
    "   gl_Position = projMatrix * mvMatrix * vertex;\n"
    "}\n";

static const char *fragmentShaderSourceCore =
    "#version 150\n"
    "in highp vec3 vert;\n"
    "in highp vec3 vertNormal;\n"
    "in highp vec3 vertColor;\n"
    "in highp vec2 vertTexCoord;\n"
    "out highp vec4 fragColor;\n"
    "uniform highp vec3 lightPos;\n"
    "uniform highp sampler2D textureId;\n"
    "uniform highp int textureEnabled;\n"
    "void main() {\n"
    "   highp vec3 L = normalize(lightPos - vert);\n"
    "   highp float NL = max(dot(normalize(vertNormal), L), 0.0);\n"
    "   highp vec3 color = vertColor;\n"
    "   if (textureEnabled == 1) {\n"
    "       color = texture(textureId, vertTexCoord).rgb;\n"
    "   }\n"
    "   highp vec3 col = clamp(color * 0.2 + color * 0.8 * NL, 0.0, 1.0);\n"
    "   fragColor = vec4(col, 1.0);\n"
    "}\n";

static const char *fragmentShaderSource =
    "varying highp vec3 vert;\n"
    "varying highp vec3 vertNormal;\n"
    "varying highp vec3 vertColor;\n"
    "varying highp vec2 vertTexCoord;\n"
    "uniform highp vec3 lightPos;\n"
    "uniform highp sampler2D textureId;\n"
    "uniform highp int textureEnabled;\n"
    "void main() {\n"
    "   highp vec3 L = normalize(lightPos - vert);\n"
    "   highp float NL = max(dot(normalize(vertNormal), L), 0.0);\n"
    "   highp vec3 color = vertColor;\n"
    "   if (textureEnabled == 1) {\n"
    "       color = texture2D(textureId, vertTexCoord).rgb;\n"
    "   }\n"
    "   highp vec3 col = clamp(color * 0.2 + color * 0.8 * NL, 0.0, 1.0);\n"
    "   gl_FragColor = vec4(col, 1.0);\n"
    "}\n";

ModelShaderProgram::ModelShaderProgram()
{
    this->addShaderFromSourceCode(QOpenGLShader::Vertex, QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile ? vertexShaderSourceCore : vertexShaderSource);
    this->addShaderFromSourceCode(QOpenGLShader::Fragment, QSurfaceFormat::defaultFormat().profile() == QSurfaceFormat::CoreProfile ? fragmentShaderSourceCore : fragmentShaderSource);
    this->bindAttributeLocation("vertex", 0);
    this->bindAttributeLocation("normal", 1);
    this->bindAttributeLocation("color", 2);
    this->bindAttributeLocation("texCoord", 3);
    this->link();

    this->bind();
    m_projMatrixLoc = this->uniformLocation("projMatrix");
    m_mvMatrixLoc = this->uniformLocation("mvMatrix");
    m_normalMatrixLoc = this->uniformLocation("normalMatrix");
    m_lightPosLoc = this->uniformLocation("lightPos");
    m_textureIdLoc = this->uniformLocation("textureId");
    m_textureEnabledLoc = this->uniformLocation("textureEnabled");
}

int ModelShaderProgram::projMatrixLoc()
{
    return m_projMatrixLoc;
}

int ModelShaderProgram::mvMatrixLoc()
{
    return m_mvMatrixLoc;
}

int ModelShaderProgram::normalMatrixLoc()
{
    return m_normalMatrixLoc;
}

int ModelShaderProgram::lightPosLoc()
{
    return m_lightPosLoc;
}

int ModelShaderProgram::textureEnabledLoc()
{
    return m_textureEnabledLoc;
}

int ModelShaderProgram::textureIdLoc()
{
    return m_textureIdLoc;
}
