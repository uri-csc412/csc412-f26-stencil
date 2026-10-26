#include "dmalloc.h"
#include <vector>

/// This magic class lets standard C++ containers use your debugging allocator,
/// instead of the system allocator. Don't worry about this
template <typename T>
class dbg_allocator {
public:
    using value_type = T;
    dbg_allocator() noexcept = default;
    dbg_allocator(const dbg_allocator<T>&) noexcept = default;
    template <typename U> dbg_allocator(dbg_allocator<U>&) noexcept {}

    T* allocate(size_t n) {
        return reinterpret_cast<T*>(dmalloc(n * sizeof(T), "?", 0));
    }
    void deallocate(T* ptr, size_t) {
        dfree(ptr, "?", 0);
    }
};
template <typename T, typename U>
inline constexpr bool operator==(const dbg_allocator<T>&, const dbg_allocator<U>&) {
    return true;
}
template <typename T, typename U>
inline constexpr bool operator!=(const dbg_allocator<T>&, const dbg_allocator<U>&) {
    return false;
}