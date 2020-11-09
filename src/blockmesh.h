#ifndef DUST3D_BLOCK_MESH_H
#define DUST3D_BLOCK_MESH_H
#include <QVector3D>
#include <vector>

class BlockMesh
{
public:
    struct Block
    {
        QVector3D fromPosition;
        double fromRadius;
        QVector3D toPosition;
        double toRadius;
    };

    BlockMesh()
    { 
    }

    ~BlockMesh()
    {
        delete m_resultVertices;
        delete m_resultQuads;
        delete m_resultFaces;
    }
    
    std::vector<QVector3D> *takeResultVertices()
    {
        std::vector<QVector3D> *resultVertices = m_resultVertices;
        m_resultVertices = nullptr;
        return resultVertices;
    }
    
    std::vector<std::vector<size_t>> *takeResultFaces()
    {
        std::vector<std::vector<size_t>> *resultFaces = m_resultFaces;
        m_resultFaces = nullptr;
        return resultFaces;
    }
    
    void addBlock(const QVector3D &fromPosition, double fromRadius, const QVector3D &toPosition, double toRadius)
    {
        Block block;
        block.fromPosition = fromPosition;
        block.fromRadius = fromRadius;
        block.toPosition = toPosition;
        block.toRadius = toRadius;
        
        m_blocks.push_back(block);
    }
    
    void build();
    
private:
    std::vector<QVector3D> *m_resultVertices = nullptr;
    std::vector<std::vector<size_t>> *m_resultFaces = nullptr;
    std::vector<std::vector<size_t>> *m_resultQuads = nullptr;
    std::vector<Block> m_blocks;
    
    void buildBlock(const Block &block);
    std::vector<size_t> buildFace(const QVector3D &origin, 
            const QVector3D &faceNormal, 
            const QVector3D &startDirection, 
            double radius);
    QVector3D calculateStartDirection(const QVector3D &direction);
};

#endif
