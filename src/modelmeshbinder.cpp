#include <QMutexLocker>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <map>
#include <QDebug>
#include <QDir>
#include "modelmeshbinder.h"
#include "ds3file.h"

ModelMeshBinder::ModelMeshBinder() :
    m_mesh(nullptr),
    m_renderTriangleVertexCount(0),
    m_renderEdgeVertexCount(0),
    m_meshUpdated(false),
    m_showWireframes(false),
    m_hasTexture(false),
    m_texture(nullptr)
{
}

ModelMeshBinder::~ModelMeshBinder()
{
    delete m_mesh;
    delete m_texture;
}

void ModelMeshBinder::updateMesh(MeshLoader *mesh)
{
    QMutexLocker lock(&m_meshMutex);
    if (mesh != m_mesh) {
        delete m_mesh;
        m_mesh = mesh;
        m_meshUpdated = true;
    }
}

void ModelMeshBinder::exportMeshAsObj(const QString &filename)
{
    QMutexLocker lock(&m_meshMutex);
    if (m_mesh) {
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream stream(&file);
            stream << "# " << Ds3FileReader::m_applicationName << endl;
            for (std::vector<QVector3D>::const_iterator it = m_mesh->vertices().begin() ; it != m_mesh->vertices().end(); ++it) {
                stream << "v " << (*it).x() << " " << (*it).y() << " " << (*it).z() << endl;
            }
            for (std::vector<std::vector<int>>::const_iterator it = m_mesh->faces().begin() ; it != m_mesh->faces().end(); ++it) {
                stream << "f";
                for (std::vector<int>::const_iterator subIt = (*it).begin() ; subIt != (*it).end(); ++subIt) {
                    stream << " " << (1 + *subIt);
                }
                stream << endl;
            }
        }
    }
}

void ModelMeshBinder::exportMeshAsObjPlusMaterials(const QString &filename)
{
    QMutexLocker lock(&m_meshMutex);
    if (m_mesh) {
        QFileInfo nameInfo(filename);
        QString mtlFilenameWithoutPath = nameInfo.baseName() + ".mtl";
        QString mtlFilename = nameInfo.path() + QDir::separator() + mtlFilenameWithoutPath;
        std::map<QString, QColor> colorNameMap;
        QString lastColorName;
        
        qDebug() << "export obj to " << filename;
        qDebug() << "export mtl to " << mtlFilename;
        
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly)) {
            QTextStream stream(&file);
            stream << "# " << Ds3FileReader::m_applicationName << endl;
            stream << "mtllib " << mtlFilenameWithoutPath << endl;
            for (std::vector<QVector3D>::const_iterator it = m_mesh->triangulatedVertices().begin() ; it != m_mesh->triangulatedVertices().end(); ++it) {
                stream << "v " << (*it).x() << " " << (*it).y() << " " << (*it).z() << endl;
            }
            for (std::vector<TriangulatedFace>::const_iterator it = m_mesh->triangulatedFaces().begin() ; it != m_mesh->triangulatedFaces().end(); ++it) {
                QString colorName = it->color.name();
                colorName = "rgb" + colorName.remove(QChar('#'));
                if (colorNameMap.find(colorName) == colorNameMap.end())
                    colorNameMap[colorName] = it->color;
                if (lastColorName != colorName) {
                    lastColorName = colorName;
                    stream << "usemtl " << colorName << endl;
                }
                stream << "f" << " " << (1 + it->indicies[0]) << " " << (1 + it->indicies[1]) << " " << (1 + it->indicies[2]) << endl;
            }
        }
        
        QFile mtlFile(mtlFilename);
        if (mtlFile.open(QIODevice::WriteOnly)) {
            QTextStream stream(&mtlFile);
            stream << "# " << Ds3FileReader::m_applicationName << endl;
            for (const auto &it: colorNameMap) {
                stream << "newmtl " << it.first << endl;
                stream << "Ka" << " " << it.second.redF() << " " << it.second.greenF() << " " << it.second.blueF() << endl;
                stream << "Kd" << " " << it.second.redF() << " " << it.second.greenF() << " " << it.second.blueF() << endl;
                stream << "Ks" << " 0.0 0.0 0.0" << endl;
                stream << "illum" << " 1" << endl;
            }
        }
    }
}

void ModelMeshBinder::initialize()
{
    m_vaoTriangle.create();
    m_vaoEdge.create();
}

void ModelMeshBinder::paint(ModelShaderProgram *program)
{
    {
        QMutexLocker lock(&m_meshMutex);
        if (m_meshUpdated) {
            if (m_mesh) {
                m_hasTexture = nullptr != m_mesh->textureImage();
                delete m_texture;
                m_texture = nullptr;
                if (m_hasTexture) {
                    m_texture = new QOpenGLTexture(*m_mesh->textureImage());
                }
                {
                    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoTriangle);
                    if (m_vboTriangle.isCreated())
                        m_vboTriangle.destroy();
                    m_vboTriangle.create();
                    m_vboTriangle.bind();
                    m_vboTriangle.allocate(m_mesh->triangleVertices(), m_mesh->triangleVertexCount() * sizeof(Vertex));
                    m_renderTriangleVertexCount = m_mesh->triangleVertexCount();
                    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
                    f->glEnableVertexAttribArray(0);
                    f->glEnableVertexAttribArray(1);
                    f->glEnableVertexAttribArray(2);
                    f->glEnableVertexAttribArray(3);
                    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), 0);
                    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), reinterpret_cast<void *>(9 * sizeof(GLfloat)));
                    m_vboTriangle.release();
                }
                {
                    QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoEdge);
                    if (m_vboEdge.isCreated())
                        m_vboEdge.destroy();
                    m_vboEdge.create();
                    m_vboEdge.bind();
                    m_vboEdge.allocate(m_mesh->edgeVertices(), m_mesh->edgeVertexCount() * sizeof(Vertex));
                    m_renderEdgeVertexCount = m_mesh->edgeVertexCount();
                    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
                    f->glEnableVertexAttribArray(0);
                    f->glEnableVertexAttribArray(1);
                    f->glEnableVertexAttribArray(2);
                    f->glEnableVertexAttribArray(3);
                    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), 0);
                    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), reinterpret_cast<void *>(9 * sizeof(GLfloat)));
                    m_vboEdge.release();
                }
            } else {
                m_renderTriangleVertexCount = 0;
                m_renderEdgeVertexCount = 0;
            }
            m_meshUpdated = false;
        }
    }
    
    if (m_showWireframes) {
        if (m_renderEdgeVertexCount > 0) {
            QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoEdge);
			QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
            program->setUniformValue(program->textureEnabledLoc(), 0);
            f->glDrawArrays(GL_LINES, 0, m_renderEdgeVertexCount);
        }
    }
    if (m_renderTriangleVertexCount > 0) {
        QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoTriangle);
		QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        if (m_hasTexture) {
            if (m_texture)
                m_texture->bind(0);
            program->setUniformValue(program->textureIdLoc(), 0);
            program->setUniformValue(program->textureEnabledLoc(), 1);
        } else {
            program->setUniformValue(program->textureEnabledLoc(), 0);
        }
        f->glDrawArrays(GL_TRIANGLES, 0, m_renderTriangleVertexCount);
    }
}

void ModelMeshBinder::cleanup()
{
    if (m_vboTriangle.isCreated())
        m_vboTriangle.destroy();
    if (m_vboEdge.isCreated())
        m_vboEdge.destroy();
    delete m_texture;
    m_texture = nullptr;
}

void ModelMeshBinder::showWireframes()
{
    m_showWireframes = true;
}

void ModelMeshBinder::hideWireframes()
{
    m_showWireframes = false;
}

bool ModelMeshBinder::isWireframesVisible()
{
    return m_showWireframes;
}
