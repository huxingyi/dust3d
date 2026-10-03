// Minimal stand-in for tbb::parallel_for over a blocked_range, on std::thread.
#ifndef DUST3D_TBB_SHIM_PARALLEL_FOR_H_
#define DUST3D_TBB_SHIM_PARALLEL_FOR_H_
#include <algorithm>
#include <thread>
#include <tbb/blocked_range.h>
#include <vector>
namespace tbb {
template <typename Value, typename Body>
void parallel_for(const blocked_range<Value>& range, const Body& body)
{
    size_t count = range.size();
    if (0 == count)
        return;
    size_t threads = std::max<size_t>(1, std::thread::hardware_concurrency());
    size_t minimum = std::max<size_t>(range.grainsize(), 256);
    threads = std::min(threads, (count + minimum - 1) / minimum);
    if (threads <= 1) {
        body(range);
        return;
    }
    std::vector<std::thread> workers;
    size_t chunk = (count + threads - 1) / threads;
    for (size_t t = 0; t < threads; ++t) {
        Value begin = range.begin() + (Value)(t * chunk);
        if (!(begin < range.end()))
            break;
        Value end = std::min(range.end(), (Value)(begin + (Value)chunk));
        workers.emplace_back([&body, begin, end]() { body(blocked_range<Value>(begin, end)); });
    }
    for (auto& worker : workers)
        worker.join();
}
}
#endif
