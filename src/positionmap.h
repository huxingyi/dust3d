#ifndef POSITION_MAP_H
#define POSITION_MAP_H
#include <map>

struct PositionMapKey
{
    int x;
    int y;
    int z;
    bool operator<(const PositionMapKey &other)  const {
        if (x < other.x) {
            return true;
        } else if (x > other.x) {
            return false;
        }
        if (y < other.y) {
            return true;
        } else if (y > other.y) {
            return false;
        }
        return z < other.z;
    }
};

template <class T>
class PositionMap
{
public:
    PositionMap(float gridSize=0.001) :
        m_intGridSize(PositionMap<T>::makeInt(gridSize))
    {
    }

    static int makeInt(float value)
    {
        return value * 1000;
    }

    PositionMapKey makeKey(float x, float y, float z)
    {
        PositionMapKey key;
        key.x = makeInt(x) / m_intGridSize;
        key.y = makeInt(y) / m_intGridSize;
        key.z = makeInt(z) / m_intGridSize;
        return key;
    }

    void addPosition(float x, float y, float z, T data)
    {
        m_map[makeKey(x, y, z)] = data;
    }

    bool findPosition(float x, float y, float z, T *data = nullptr)
    {
        const auto &result = m_map.find(makeKey(x, y, z));
        if (result == m_map.end())
            return false;
        if (data)
            *data = result->second;
        return true;
    }
    
    void removePosition(float x, float y, float z)
    {
        m_map.erase(makeKey(x, y, z));
    }
    
    std::map<PositionMapKey, T> &map()
    {
        return m_map;
    }

private:
    std::map<PositionMapKey, T> m_map;
    int m_intGridSize;
};

#endif
