#include <QMutexLocker>
#include <QFile>
#include <QTextStream>
#include "modelmeshbinder.h"
#include "ds3file.h"

ModelMeshBinder::ModelMeshBinder() :
    m_renderTriangleVertexCount(0),
    m_renderEdgeVertexCount(0),
    m_mesh(NULL),
    m_meshUpdated(false)
{
}

ModelMeshBinder::~ModelMeshBinder()
{
    delete m_mesh;
}

void ModelMeshBinder::updateMesh(Mesh *mesh)
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
            for (std::vector<const QVector3D>::iterator it = m_mesh->vertices().begin() ; it != m_mesh->vertices().end(); ++it) {
                stream << "v " << (*it).x() << " " << (*it).y() << " " << (*it).z() << endl;
            }
            for (std::vector<const std::vector<int>>::iterator it = m_mesh->faces().begin() ; it != m_mesh->faces().end(); ++it) {
                stream << "f";
                for (std::vector<const int>::iterator subIt = (*it).begin() ; subIt != (*it).end(); ++subIt) {
                    stream << " " << (1 + *subIt);
                }
                stream << endl;
            }
        }
    }
}

void ModelMeshBinder::initialize()
{
    m_vaoTriangle.create();
    m_vaoEdge.create();
}

void ModelMeshBinder::paint()
{
    {
        QMutexLocker lock(&m_meshMutex);
        if (m_meshUpdated) {
            if (m_mesh) {
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
                    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), 0);
                    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
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
                    f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), 0);
                    f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
                    f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(GLfloat), reinterpret_cast<void *>(6 * sizeof(GLfloat)));
                    m_vboEdge.release();
                }
            } else {
                m_renderTriangleVertexCount = 0;
                m_renderEdgeVertexCount = 0;
            }
            m_meshUpdated = false;
        }
    }

    if (m_renderEdgeVertexCount > 0) {
        QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoEdge);
        glDrawArrays(GL_LINES, 0, m_renderEdgeVertexCount);
    }
    if (m_renderTriangleVertexCount > 0) {
        QOpenGLVertexArrayObject::Binder vaoBinder(&m_vaoTriangle);
        glDrawArrays(GL_TRIANGLES, 0, m_renderTriangleVertexCount);
    }
}

void ModelMeshBinder::cleanup()
{
    if (m_vboTriangle.isCreated())
        m_vboTriangle.destroy();
    if (m_vboEdge.isCreated())
        m_vboEdge.destroy();
}
