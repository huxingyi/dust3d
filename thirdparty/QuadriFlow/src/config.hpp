#ifndef CONFIG_H_
#define CONFIG_H_

// Move settings to cmake to make CMake happy :)

// #define WITH_SCALE
// #define WITH_CUDA

const int GRAIN_SIZE = 1024;

#ifdef LOG_OUTPUT

#define lprintf(...) printf(__VA_ARGS__)
#define lputs(...) puts(__VA_ARGS__)

#else

#define lprintf(...) void(0)
#define lputs(...) void(0)

#endif

#include <chrono>

namespace qflow {

// simulation of Windows GetTickCount()
unsigned long long inline GetCurrentTime64() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}
} // namespace qflow

// The following make_unique is to fix CXX14 in CXX11
#ifndef _WIN32
#include <memory>
namespace std {
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}
#endif

#endif
