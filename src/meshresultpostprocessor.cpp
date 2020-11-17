#include <QGuiApplication>
#include "meshresultpostprocessor.h"
#include "uvunwrap.h"
#include "triangletangentresolve.h"

MeshResultPostProcessor::MeshResultPostProcessor(const Object &object)
{
    m_object = new Object;
    *m_object = object;
}

MeshResultPostProcessor::~MeshResultPostProcessor()
{
    delete m_object;
}

Object *MeshResultPostProcessor::takePostProcessedObject()
{
    Object *object = m_object;
    m_object = nullptr;
    return object;
}

void MeshResultPostProcessor::poseProcess()
{
#ifndef NDEBUG
    return;
#endif
    if (!m_object->nodes.empty()) {
        {
            std::vector<std::vector<QVector2D>> triangleVertexUvs;
            std::set<int> seamVertices;
            std::map<QUuid, std::vector<QRectF>> partUvRects;
            uvUnwrap(*m_object, triangleVertexUvs, seamVertices, partUvRects);
            m_object->setTriangleVertexUvs(triangleVertexUvs);
            m_object->setPartUvRects(partUvRects);
        }
        
        {
            std::vector<QVector3D> triangleTangents;
            triangleTangentResolve(*m_object, triangleTangents);
            m_object->setTriangleTangents(triangleTangents);
        }
    }
}

void MeshResultPostProcessor::process()
{
    poseProcess();

    emit finished();
}
