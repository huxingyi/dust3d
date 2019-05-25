#ifndef NODEMESH_COMBINER_H
#define NODEMESH_COMBINER_H
#include <QVector3D>
#include <vector>

namespace nodemesh 
{

class Combiner
{
public:
    enum class Method
    {
        Union,
        Diff
    };
    
    enum class Source
    {
        None,
        First,
        Second
    };

    class Mesh
    {
    public:
        Mesh() = default;
        Mesh(const std::vector<QVector3D> &vertices, const std::vector<std::vector<size_t>> &faces, bool removeSelfIntersects=true);
        Mesh(const Mesh &other);
        ~Mesh();
        void fetch(std::vector<QVector3D> &vertices, std::vector<std::vector<size_t>> &faces) const;
        bool isNull() const;
        bool isSelfIntersected() const;
        
        friend Combiner;
        
    private:
        void *m_privateData = nullptr;
        bool m_isSelfIntersected = false;
        
        void validate();
    };
    
    static Mesh *combine(const Mesh &firstMesh, const Mesh &secondMesh, Method method,
        std::vector<std::pair<Source, size_t>> *combinedVerticesComeFrom=nullptr);
};

}

#endif
