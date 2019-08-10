#include <simpleuv/meshdatatype.h>
#include <Eigen/Dense>

namespace simpleuv
{

float dotProduct(const Vector3 &first, const Vector3 &second)
{
    Eigen::Vector3d v(first.xyz[0], first.xyz[1], first.xyz[2]);
    Eigen::Vector3d w(second.xyz[0], second.xyz[1], second.xyz[2]);
    return v.dot(w);
}

Vector3 crossProduct(const Vector3 &first, const Vector3 &second)
{
    Eigen::Vector3d v(first.xyz[0], first.xyz[1], first.xyz[2]);
    Eigen::Vector3d w(second.xyz[0], second.xyz[1], second.xyz[2]);
    auto u = v.cross(w);
    Vector3 result;
    result.xyz[0] = u.x();
    result.xyz[1] = u.y();
    result.xyz[2] = u.z();
    return result;
}

}
