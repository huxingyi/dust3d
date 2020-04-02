#include "normalanddepthmapsgenerator.h"

NormalAndDepthMapsGenerator::NormalAndDepthMapsGenerator(ModelWidget *modelWidget)
{
    m_viewPortSize = QSize(modelWidget->widthInPixels(),
		modelWidget->heightInPixels());
    m_normalMapRender = createOfflineRender(modelWidget, 1);
    m_depthMapRender = createOfflineRender(modelWidget, 2);
}

void NormalAndDepthMapsGenerator::updateMesh(MeshLoader *mesh)
{
    if (nullptr == mesh) {
        m_normalMapRender->updateMesh(nullptr);
        m_depthMapRender->updateMesh(nullptr);
        return;
    }
    m_normalMapRender->updateMesh(new MeshLoader(*mesh));
    m_depthMapRender->updateMesh(mesh);
}

void NormalAndDepthMapsGenerator::setRenderThread(QThread *thread)
{
    //m_normalMapRender->setRenderThread(thread);
    //m_depthMapRender->setRenderThread(thread);
}

ModelOfflineRender *NormalAndDepthMapsGenerator::createOfflineRender(ModelWidget *modelWidget, int purpose)
{
    ModelOfflineRender *offlineRender = new ModelOfflineRender(modelWidget->format());
    offlineRender->setXRotation(modelWidget->xRot());
    offlineRender->setYRotation(modelWidget->yRot());
    offlineRender->setZRotation(modelWidget->zRot());
    offlineRender->setRenderPurpose(purpose);
    return offlineRender;
}

NormalAndDepthMapsGenerator::~NormalAndDepthMapsGenerator()
{
    delete m_normalMapRender;
    delete m_depthMapRender;
    delete m_normalMap;
    delete m_depthMap;
}

void NormalAndDepthMapsGenerator::generate()
{
    m_normalMap = new QImage(m_normalMapRender->toImage(m_viewPortSize));
    m_depthMap = new QImage(m_depthMapRender->toImage(m_viewPortSize));
}

void NormalAndDepthMapsGenerator::process()
{
    generate();
    emit finished();
}

QImage *NormalAndDepthMapsGenerator::takeNormalMap()
{
    QImage *normalMap = m_normalMap;
    m_normalMap = nullptr;
    return normalMap;
}

QImage *NormalAndDepthMapsGenerator::takeDepthMap()
{
    QImage *depthMap = m_depthMap;
    m_depthMap = nullptr;
    return depthMap;
}
