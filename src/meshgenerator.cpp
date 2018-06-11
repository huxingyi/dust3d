#include <vector>
#include <QGuiApplication>
#include "meshgenerator.h"
#include "dust3dutil.h"
#include "skeletondocument.h"
#include "meshlite.h"
#include "modelofflinerender.h"
#include "meshutil.h"
#include "theme.h"
#include "positionmap.h"

bool MeshGenerator::enableDebug = false;
bool MeshGenerator::disableUnion = false;

MeshGenerator::MeshGenerator(SkeletonSnapshot *snapshot, QThread *thread) :
    m_snapshot(snapshot),
    m_mesh(nullptr),
    m_preview(nullptr),
    m_requirePreview(false),
    m_previewRender(nullptr),
    m_thread(thread),
    m_meshResultContext(nullptr)
{
}

MeshGenerator::~MeshGenerator()
{
    delete m_snapshot;
    delete m_mesh;
    delete m_preview;
    for (const auto &partPreviewIt: m_partPreviewMap) {
        delete partPreviewIt.second;
    }
    for (const auto &render: m_partPreviewRenderMap) {
        delete render.second;
    }
    delete m_previewRender;
    delete m_meshResultContext;
}

void MeshGenerator::addPreviewRequirement()
{
    m_requirePreview = true;
    if (nullptr == m_previewRender) {
        m_previewRender = new ModelOfflineRender;
        m_previewRender->setRenderThread(m_thread);
    }
}

void MeshGenerator::addPartPreviewRequirement(const QString &partId)
{
    //qDebug() << "addPartPreviewRequirement:" << partId;
    m_requirePartPreviewMap.insert(partId);
    if (m_partPreviewRenderMap.find(partId) == m_partPreviewRenderMap.end()) {
        ModelOfflineRender *render = new ModelOfflineRender;
        render->setRenderThread(m_thread);
        m_partPreviewRenderMap[partId] = render;
    }
}

MeshLoader *MeshGenerator::takeResultMesh()
{
    MeshLoader *resultMesh = m_mesh;
    m_mesh = nullptr;
    return resultMesh;
}

QImage *MeshGenerator::takePreview()
{
    QImage *resultPreview = m_preview;
    m_preview = nullptr;
    return resultPreview;
}

QImage *MeshGenerator::takePartPreview(const QString &partId)
{
    QImage *resultImage = m_partPreviewMap[partId];
    m_partPreviewMap[partId] = nullptr;
    return resultImage;
}

MeshResultContext *MeshGenerator::takeMeshResultContext()
{
    MeshResultContext *meshResultContext = m_meshResultContext;
    m_meshResultContext = nullptr;
    return meshResultContext;
}

void MeshGenerator::resolveBoundingBox(QRectF *mainProfile, QRectF *sideProfile, const QString &partId)
{
    m_snapshot->resolveBoundingBox(mainProfile, sideProfile, partId);
}

void MeshGenerator::loadVertexSourcesToMeshResultContext(void *meshliteContext, int meshId, int bmeshId)
{
    int vertexCount = meshlite_get_vertex_count(meshliteContext, meshId);
    int positionBufferLen = vertexCount * 3;
    float *positionBuffer = new float[positionBufferLen];
    int positionCount = meshlite_get_vertex_position_array(meshliteContext, meshId, positionBuffer, positionBufferLen) / 3;
    int *sourceBuffer = new int[positionBufferLen];
    int sourceCount = meshlite_get_vertex_source_array(meshliteContext, meshId, sourceBuffer, positionBufferLen);
    Q_ASSERT(positionCount == sourceCount);
    for (int i = 0, positionIndex = 0; i < positionCount; i++, positionIndex+=3) {
        BmeshVertex vertex;
        vertex.bmeshId = bmeshId;
        vertex.nodeId = sourceBuffer[i];
        vertex.position = QVector3D(positionBuffer[positionIndex + 0], positionBuffer[positionIndex + 1], positionBuffer[positionIndex + 2]);
        m_meshResultContext->bmeshVertices.push_back(vertex);
    }
    delete[] positionBuffer;
    delete[] sourceBuffer;
}

void MeshGenerator::loadGeneratedPositionsToMeshResultContext(void *meshliteContext, int triangulatedMeshId)
{
    int vertexCount = meshlite_get_vertex_count(meshliteContext, triangulatedMeshId);
    int positionBufferLen = vertexCount * 3;
    float *positionBuffer = new float[positionBufferLen];
    int positionCount = meshlite_get_vertex_position_array(meshliteContext, triangulatedMeshId, positionBuffer, positionBufferLen) / 3;
    std::map<int, int> verticesMap;
    for (int i = 0, positionIndex = 0; i < positionCount; i++, positionIndex+=3) {
        ResultVertex vertex;
        vertex.position = QVector3D(positionBuffer[positionIndex + 0], positionBuffer[positionIndex + 1], positionBuffer[positionIndex + 2]);
        verticesMap[i] = m_meshResultContext->vertices.size();
        m_meshResultContext->vertices.push_back(vertex);
    }
    int faceCount = meshlite_get_face_count(meshliteContext, triangulatedMeshId);
    int triangleIndexBufferLen = faceCount * 3;
    int *triangleIndexBuffer = new int[triangleIndexBufferLen];
    int triangleCount = meshlite_get_triangle_index_array(meshliteContext, triangulatedMeshId, triangleIndexBuffer, triangleIndexBufferLen) / 3;
    int triangleNormalBufferLen = faceCount * 3;
    float *normalBuffer = new float[triangleNormalBufferLen];
    int normalCount = meshlite_get_triangle_normal_array(meshliteContext, triangulatedMeshId, normalBuffer, triangleNormalBufferLen) / 3;
    Q_ASSERT(triangleCount == normalCount);
    for (int i = 0, triangleVertIndex = 0, normalIndex=0; i < triangleCount; i++, triangleVertIndex+=3, normalIndex += 3) {
        ResultTriangle triangle;
        triangle.indicies[0] = verticesMap[triangleIndexBuffer[triangleVertIndex + 0]];
        triangle.indicies[1] = verticesMap[triangleIndexBuffer[triangleVertIndex + 1]];
        triangle.indicies[2] = verticesMap[triangleIndexBuffer[triangleVertIndex + 2]];
        triangle.normal = QVector3D(normalBuffer[normalIndex + 0], normalBuffer[normalIndex + 1], normalBuffer[normalIndex + 2]);
        m_meshResultContext->triangles.push_back(triangle);
    }
    delete[] positionBuffer;
    delete[] triangleIndexBuffer;
    delete[] normalBuffer;
}

void MeshGenerator::process()
{
    if (nullptr == m_snapshot)
        return;
    
    m_meshResultContext = new MeshResultContext;
    
    void *meshliteContext = meshlite_create_context();
    std::map<QString, int> partBmeshMap;
    std::map<QString, int> bmeshNodeMap;
    
    QRectF mainProfile, sideProfile;
    resolveBoundingBox(&mainProfile, &sideProfile);
    float longHeight = mainProfile.height();
    if (mainProfile.width() > longHeight)
        longHeight = mainProfile.width();
    if (sideProfile.width() > longHeight)
        longHeight = sideProfile.width();
    float mainProfileMiddleX = mainProfile.x() + mainProfile.width() / 2;
    float sideProfileMiddleX = sideProfile.x() + sideProfile.width() / 2;
    float mainProfileMiddleY = mainProfile.y() + mainProfile.height() / 2;
    float originX = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originX").toFloat();
    float originY = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originY").toFloat();
    float originZ = valueOfKeyInMapOrEmpty(m_snapshot->canvas, "originZ").toFloat();
    bool originSettled = false;
    if (originX > 0 && originY > 0 && originZ > 0) {
        //qDebug() << "Use settled origin: " << originX << originY << originZ << " calculated:" << mainProfileMiddleX << mainProfileMiddleY << sideProfileMiddleX;
        mainProfileMiddleX = originX;
        mainProfileMiddleY = originY;
        sideProfileMiddleX = originZ;
        originSettled = true;
    } else {
        //qDebug() << "No settled origin, calculated:" << mainProfileMiddleX << mainProfileMiddleY << sideProfileMiddleX;
    }
    
    std::map<QString, QColor> partColorMap;
    std::map<QString, bool> partMirrorFlagMap;
    
    for (const auto &partIdIt: m_snapshot->partIdList) {
        const auto &part = m_snapshot->parts.find(partIdIt);
        if (part == m_snapshot->parts.end())
            continue;
        QString disabledString = valueOfKeyInMapOrEmpty(part->second, "disabled");
        bool isDisabled = isTrueValueString(disabledString);
        if (isDisabled)
            continue;
        bool subdived = isTrueValueString(valueOfKeyInMapOrEmpty(part->second, "subdived"));
        int bmeshId = meshlite_bmesh_create(meshliteContext);
        if (subdived)
            meshlite_bmesh_set_cut_subdiv_count(meshliteContext, bmeshId, 1);
        if (isTrueValueString(valueOfKeyInMapOrEmpty(part->second, "rounded")))
            meshlite_bmesh_set_round_way(meshliteContext, bmeshId, 1);
        partMirrorFlagMap[partIdIt] = isTrueValueString(valueOfKeyInMapOrEmpty(part->second, "xMirrored"));
        QString colorString = valueOfKeyInMapOrEmpty(part->second, "color");
        QColor partColor = colorString.isEmpty() ? Theme::white : QColor(colorString);
        partColorMap[partIdIt] = partColor;
        QString thicknessString = valueOfKeyInMapOrEmpty(part->second, "deformThickness");
        if (!thicknessString.isEmpty())
            meshlite_bmesh_set_deform_thickness(meshliteContext, bmeshId, thicknessString.toFloat());
        QString widthString = valueOfKeyInMapOrEmpty(part->second, "deformWidth");
        if (!widthString.isEmpty())
            meshlite_bmesh_set_deform_width(meshliteContext, bmeshId, widthString.toFloat());
        if (MeshGenerator::enableDebug)
            meshlite_bmesh_enable_debug(meshliteContext, bmeshId, 1);
        partBmeshMap[partIdIt] = bmeshId;
    }
    
    for (const auto &edgeIt: m_snapshot->edges) {
        QString partId = valueOfKeyInMapOrEmpty(edgeIt.second, "partId");
        QString fromNodeId = valueOfKeyInMapOrEmpty(edgeIt.second, "from");
        QString toNodeId = valueOfKeyInMapOrEmpty(edgeIt.second, "to");
        //qDebug() << "Processing edge " << fromNodeId << "<=>" << toNodeId;
        const auto fromIt = m_snapshot->nodes.find(fromNodeId);
        const auto toIt = m_snapshot->nodes.find(toNodeId);
        if (fromIt == m_snapshot->nodes.end() || toIt == m_snapshot->nodes.end())
            continue;
        const auto partBmeshIt = partBmeshMap.find(partId);
        if (partBmeshIt == partBmeshMap.end())
            continue;
        int bmeshId = partBmeshIt->second;
        
        int bmeshFromNodeId = 0;
        const auto bmeshFromIt = bmeshNodeMap.find(fromNodeId);
        if (bmeshFromIt == bmeshNodeMap.end()) {
            float radius = valueOfKeyInMapOrEmpty(fromIt->second, "radius").toFloat() / longHeight;
            float x = (valueOfKeyInMapOrEmpty(fromIt->second, "x").toFloat() - mainProfileMiddleX) / longHeight;
            float y = (mainProfileMiddleY - valueOfKeyInMapOrEmpty(fromIt->second, "y").toFloat()) / longHeight;
            float z = (sideProfileMiddleX - valueOfKeyInMapOrEmpty(fromIt->second, "z").toFloat()) / longHeight;
            bmeshFromNodeId = meshlite_bmesh_add_node(meshliteContext, bmeshId, x, y, z, radius);
            //qDebug() << "bmeshId[" << bmeshId << "] add node[" << bmeshFromNodeId << "]" << radius << x << y << z;
            bmeshNodeMap[fromNodeId] = bmeshFromNodeId;
            
            SkeletonBoneMark boneMark = SkeletonBoneMarkFromString(valueOfKeyInMapOrEmpty(fromIt->second, "boneMark").toUtf8().constData());
            
            BmeshNode bmeshNode;
            bmeshNode.bmeshId = bmeshId;
            bmeshNode.origin = QVector3D(x, y, z);
            bmeshNode.radius = radius;
            bmeshNode.nodeId = bmeshFromNodeId;
            bmeshNode.color = partColorMap[partId];
            bmeshNode.boneMark = boneMark;
            m_meshResultContext->bmeshNodes.push_back(bmeshNode);
            
            if (partMirrorFlagMap[partId]) {
                bmeshNode.bmeshId = -bmeshId;
                bmeshNode.origin.setX(-x);
                m_meshResultContext->bmeshNodes.push_back(bmeshNode);
            }
        } else {
            bmeshFromNodeId = bmeshFromIt->second;
            //qDebug() << "bmeshId[" << bmeshId << "] use existed node[" << bmeshFromNodeId << "]";
        }
        
        int bmeshToNodeId = 0;
        const auto bmeshToIt = bmeshNodeMap.find(toNodeId);
        if (bmeshToIt == bmeshNodeMap.end()) {
            float radius = valueOfKeyInMapOrEmpty(toIt->second, "radius").toFloat() / longHeight;
            float x = (valueOfKeyInMapOrEmpty(toIt->second, "x").toFloat() - mainProfileMiddleX) / longHeight;
            float y = (mainProfileMiddleY - valueOfKeyInMapOrEmpty(toIt->second, "y").toFloat()) / longHeight;
            float z = (sideProfileMiddleX - valueOfKeyInMapOrEmpty(toIt->second, "z").toFloat()) / longHeight;
            bmeshToNodeId = meshlite_bmesh_add_node(meshliteContext, bmeshId, x, y, z, radius);
            //qDebug() << "bmeshId[" << bmeshId << "] add node[" << bmeshToNodeId << "]" << radius << x << y << z;
            bmeshNodeMap[toNodeId] = bmeshToNodeId;
            
            SkeletonBoneMark boneMark = SkeletonBoneMarkFromString(valueOfKeyInMapOrEmpty(toIt->second, "boneMark").toUtf8().constData());
            
            BmeshNode bmeshNode;
            bmeshNode.bmeshId = bmeshId;
            bmeshNode.origin = QVector3D(x, y, z);
            bmeshNode.radius = radius;
            bmeshNode.nodeId = bmeshToNodeId;
            bmeshNode.color = partColorMap[partId];
            bmeshNode.boneMark = boneMark;
            m_meshResultContext->bmeshNodes.push_back(bmeshNode);
            
            if (partMirrorFlagMap[partId]) {
                bmeshNode.bmeshId = -bmeshId;
                bmeshNode.origin.setX(-x);
                m_meshResultContext->bmeshNodes.push_back(bmeshNode);
            }
        } else {
            bmeshToNodeId = bmeshToIt->second;
            //qDebug() << "bmeshId[" << bmeshId << "] use existed node[" << bmeshToNodeId << "]";
        }
        
        meshlite_bmesh_add_edge(meshliteContext, bmeshId, bmeshFromNodeId, bmeshToNodeId);
        
        BmeshEdge bmeshEdge;
        bmeshEdge.fromBmeshId = bmeshId;
        bmeshEdge.fromNodeId = bmeshFromNodeId;
        bmeshEdge.toBmeshId = bmeshId;
        bmeshEdge.toNodeId = bmeshToNodeId;
        m_meshResultContext->bmeshEdges.push_back(bmeshEdge);
        
        if (partMirrorFlagMap[partId]) {
            BmeshEdge bmeshEdge;
            bmeshEdge.fromBmeshId = -bmeshId;
            bmeshEdge.fromNodeId = bmeshFromNodeId;
            bmeshEdge.toBmeshId = -bmeshId;
            bmeshEdge.toNodeId = bmeshToNodeId;
            m_meshResultContext->bmeshEdges.push_back(bmeshEdge);
        }
    }
    
    for (const auto &nodeIt: m_snapshot->nodes) {
        QString partId = valueOfKeyInMapOrEmpty(nodeIt.second, "partId");
        const auto partBmeshIt = partBmeshMap.find(partId);
        if (partBmeshIt == partBmeshMap.end())
            continue;
        const auto nodeBmeshIt = bmeshNodeMap.find(nodeIt.first);
        if (nodeBmeshIt != bmeshNodeMap.end())
            continue;
        int bmeshId = partBmeshIt->second;
        float radius = valueOfKeyInMapOrEmpty(nodeIt.second, "radius").toFloat() / longHeight;
        float x = (valueOfKeyInMapOrEmpty(nodeIt.second, "x").toFloat() - mainProfileMiddleX) / longHeight;
        float y = (mainProfileMiddleY - valueOfKeyInMapOrEmpty(nodeIt.second, "y").toFloat()) / longHeight;
        float z = (sideProfileMiddleX - valueOfKeyInMapOrEmpty(nodeIt.second, "z").toFloat()) / longHeight;
        int bmeshNodeId = meshlite_bmesh_add_node(meshliteContext, bmeshId, x, y, z, radius);
        //qDebug() << "bmeshId[" << bmeshId << "] add lonely node[" << bmeshNodeId << "]" << radius << x << y << z;
        bmeshNodeMap[nodeIt.first] = bmeshNodeId;
        
        SkeletonBoneMark boneMark = SkeletonBoneMarkFromString(valueOfKeyInMapOrEmpty(nodeIt.second, "boneMark").toUtf8().constData());
        
        BmeshNode bmeshNode;
        bmeshNode.bmeshId = bmeshId;
        bmeshNode.origin = QVector3D(x, y, z);
        bmeshNode.radius = radius;
        bmeshNode.nodeId = bmeshNodeId;
        bmeshNode.color = partColorMap[partId];
        bmeshNode.boneMark = boneMark;
        m_meshResultContext->bmeshNodes.push_back(bmeshNode);
        
        if (partMirrorFlagMap[partId]) {
            bmeshNode.bmeshId = -bmeshId;
            bmeshNode.origin.setX(-x);
            m_meshResultContext->bmeshNodes.push_back(bmeshNode);
        }
    }
    
    bool broken = false;
    
    std::vector<int> meshIds;
    std::vector<int> subdivMeshIds;
    for (const auto &partIdIt: m_snapshot->partIdList) {
        const auto &part = m_snapshot->parts.find(partIdIt);
        if (part == m_snapshot->parts.end())
            continue;
        QString disabledString = valueOfKeyInMapOrEmpty(part->second, "disabled");
        bool isDisabled = isTrueValueString(disabledString);
        if (isDisabled)
            continue;
        int bmeshId = partBmeshMap[partIdIt];
        int meshId = meshlite_bmesh_generate_mesh(meshliteContext, bmeshId);
        if (meshlite_bmesh_error_count(meshliteContext, bmeshId) != 0)
            broken = true;
        bool xMirrored = isTrueValueString(valueOfKeyInMapOrEmpty(part->second, "xMirrored"));
        loadVertexSourcesToMeshResultContext(meshliteContext, meshId, bmeshId);
        QColor modelColor = partColorMap[partIdIt];
        int xMirroredMeshId = 0;
        if (xMirrored) {
            if (xMirrored) {
                xMirroredMeshId = meshlite_mirror_in_x(meshliteContext, meshId, 0);
                loadVertexSourcesToMeshResultContext(meshliteContext, xMirroredMeshId, -bmeshId);
            }
        }
        if (m_requirePartPreviewMap.find(partIdIt) != m_requirePartPreviewMap.end()) {
            ModelOfflineRender *render = m_partPreviewRenderMap[partIdIt];
            int trimedMeshId = meshlite_trim(meshliteContext, meshId, 1);
            render->updateMesh(new MeshLoader(meshliteContext, trimedMeshId, -1, modelColor));
            QImage *image = new QImage(render->toImage(QSize(Theme::previewImageRenderSize, Theme::previewImageRenderSize)));
            m_partPreviewMap[partIdIt] = image;
        }
        meshIds.push_back(meshId);
        if (xMirroredMeshId)
            meshIds.push_back(xMirroredMeshId);
    }
    
    if (!subdivMeshIds.empty()) {
        int mergedMeshId = 0;
        if (subdivMeshIds.size() > 1) {
            int errorCount = 0;
            mergedMeshId = unionMeshs(meshliteContext, subdivMeshIds, &errorCount);
            if (errorCount)
                broken = true;
        } else {
            mergedMeshId = subdivMeshIds[0];
        }
        //if (mergedMeshId > 0)
        //    mergedMeshId = meshlite_combine_coplanar_faces(meshliteContext, mergedMeshId);
        if (mergedMeshId > 0) {
            int errorCount = 0;
            int subdivedMeshId = subdivMesh(meshliteContext, mergedMeshId, &errorCount);
            if (errorCount > 0)
                broken = true;
            if (subdivedMeshId > 0)
                mergedMeshId = subdivedMeshId;
            else
                broken = true;
        }
        if (mergedMeshId > 0)
            meshIds.push_back(mergedMeshId);
        else
            broken = true;
    }
    
    int mergedMeshId = 0;
    if (meshIds.size() > 0) {
        int errorCount = 0;
        if (disableUnion)
            mergedMeshId = mergeMeshs(meshliteContext, meshIds);
        else
            mergedMeshId = unionMeshs(meshliteContext, meshIds, &errorCount);
        if (errorCount)
            broken = true;
        else if (mergedMeshId > 0)
            mergedMeshId = meshlite_combine_coplanar_faces(meshliteContext, mergedMeshId);
            if (mergedMeshId > 0)
                mergedMeshId = meshlite_fix_hole(meshliteContext, mergedMeshId);
    }
    
    if (mergedMeshId > 0) {
        if (m_requirePreview) {
            m_previewRender->updateMesh(new MeshLoader(meshliteContext, mergedMeshId));
            QImage *image = new QImage(m_previewRender->toImage(QSize(Theme::previewImageRenderSize, Theme::previewImageRenderSize)));
            m_preview = image;
        }
        int finalMeshId = mergedMeshId;
        int triangulatedFinalMeshId = meshlite_triangulate(meshliteContext, mergedMeshId);
        loadGeneratedPositionsToMeshResultContext(meshliteContext, triangulatedFinalMeshId);
        m_mesh = new MeshLoader(meshliteContext, finalMeshId, triangulatedFinalMeshId, broken ? Theme::broken : Theme::white, &m_meshResultContext->triangleColors());
    }
    
    if (m_previewRender) {
        m_previewRender->setRenderThread(QGuiApplication::instance()->thread());
    }
    
    for (auto &partPreviewRender: m_partPreviewRenderMap) {
        partPreviewRender.second->setRenderThread(QGuiApplication::instance()->thread());
    }
    
    meshlite_destroy_context(meshliteContext);
    
    this->moveToThread(QGuiApplication::instance()->thread());
    
    emit finished();
}
