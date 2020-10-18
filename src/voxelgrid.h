#ifndef DUST3D_VOXEL_GRID_H
#define DUST3D_VOXEL_GRID_H
#include <QtGlobal>
#include <unordered_map>

template<typename T>
class VoxelGrid
{
public:
    struct Voxel
    {
        qint16 x;
        qint16 y;
        qint16 z;
    };
    
    struct VoxelHash
    {
        size_t operator()(const Voxel &voxel) const
        {
            return ((size_t)voxel.x ^ ((size_t)voxel.y << 1)) ^ (size_t)voxel.z;
        }
    };

    struct VoxelEqual 
    {
        bool operator()(const Voxel &left, const Voxel &right) const
        {
            return (left.x == right.x) && 
                (left.y == right.y) && 
                (left.z == right.z);
        }
    };
    
    T query(qint16 x, qint16 y, qint16 z)
    {
        auto findResult = m_grid.find({x, y, z});
        if (findResult == m_grid.end())
            return m_nullValue;
        return findResult->second;
    }
    
    T add(qint16 x, qint16 y, qint16 z, T value)
    {
        auto insertResult = m_grid.insert(std::make_pair(Voxel {x, y, z}, value));
        if (insertResult.second) {
            insertResult.first->second = m_nullValue + value;
            return insertResult.first->second;
        }
        insertResult.first->second = insertResult.first->second + value;
        return insertResult.first->second;
    }
    
    T sub(qint16 x, qint16 y, qint16 z, T value)
    {
        auto findResult = m_grid.find({x, y, z});
        if (findResult == m_grid.end())
            return m_nullValue;
        findResult->second = findResult->second - value;
        if (findResult->second == m_nullValue) {
            m_grid.erase(findResult);
            return m_nullValue;
        }
        return findResult->second;
    }
    
    void reset(qint16 x, qint16 y, qint16 z)
    {
        auto findResult = m_grid.find({x, y, z});
        if (findResult == m_grid.end())
            return;
        m_grid.erase(findResult);
    }
    
    void setNullValue(const T &nullValue)
    {
        m_nullValue = nullValue;
    }
    
private:
    std::unordered_map<Voxel, T, VoxelHash, VoxelEqual> m_grid;
    T m_nullValue = T();
};

#endif
