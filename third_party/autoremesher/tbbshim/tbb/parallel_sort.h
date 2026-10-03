// Minimal stand-in for tbb::parallel_sort (sequential).
#ifndef DUST3D_TBB_SHIM_PARALLEL_SORT_H_
#define DUST3D_TBB_SHIM_PARALLEL_SORT_H_
#include <algorithm>
#include <cstdint>
namespace tbb {
template <typename Iterator>
void parallel_sort(Iterator begin, Iterator end) { std::sort(begin, end); }
template <typename Iterator, typename Compare>
void parallel_sort(Iterator begin, Iterator end, const Compare& compare) { std::sort(begin, end, compare); }
}
#endif
