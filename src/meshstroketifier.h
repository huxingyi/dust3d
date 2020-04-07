#ifndef DUST3D_MESH_STROKETIFIER_H
#define DUST3D_MESH_STROKETIFIER_H
#include <QObject>
#include <QVector3D>
#include <QMatrix4x4>
#include <vector>

class MeshStroketifier : public QObject
{
    Q_OBJECT
public:
    struct Node
    {
        QVector3D position;
        float radius;
    };
	void setCutRotation(float cutRotation);
    bool prepare(const std::vector<Node> &strokeNodes, 
        const std::vector<QVector3D> &vertices);
    void stroketify(std::vector<QVector3D> *vertices);
    void stroketify(std::vector<Node> *nodes);
private:
	float m_cutRotation = 0.0;

    QVector3D m_modelOrigin;
    float m_modelLength = 0.0;
    float m_scaleAmount = 1.0;
    QVector3D m_modelAlignDirection;
    std::vector<QVector3D> m_modelJointPositions;
    std::vector<QMatrix4x4> m_modelTransforms;

    static float calculateStrokeLengths(const std::vector<Node> &strokeNodes,
        std::vector<float> *lengths);
    static void calculateBoundingBox(const std::vector<QVector3D> &vertices,
        float *minX, float *maxX,
        float *minY, float *maxY,
        float *minZ, float *maxZ);
        
    void translate(std::vector<QVector3D> *vertices);
    void scale(std::vector<QVector3D> *vertices);
    void deform(std::vector<QVector3D> *vertices);
};

#endif
