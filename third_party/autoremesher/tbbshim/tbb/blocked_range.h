// Minimal stand-in for the parts of oneTBB that AutoRemesher uses (Dust3D has no TBB).
#ifndef DUST3D_TBB_SHIM_BLOCKED_RANGE_H_
#define DUST3D_TBB_SHIM_BLOCKED_RANGE_H_
#include <cstddef>
#include <cstdint>
namespace tbb {
template <typename Value>
class blocked_range {
public:
    blocked_range(Value begin, Value end, size_t grainsize = 1)
        : m_begin(begin)
        , m_end(end)
        , m_grainsize(grainsize)
    {
    }
    Value begin() const { return m_begin; }
    Value end() const { return m_end; }
    size_t size() const { return (size_t)(m_end - m_begin); }
    bool empty() const { return !(m_begin < m_end); }
    size_t grainsize() const { return m_grainsize; }
private:
    Value m_begin;
    Value m_end;
    size_t m_grainsize;
};
}
#endif
