#define DMALLOC_DISABLE 1
#include "dmalloc.h"
//#include <unordered_map>
//#include <vector>
#include <sys/mman.h>


// This file contains a base memory allocator guaranteed not to
// overwrite freed allocations. No need to understand it.

//using base_allocation = std::pair<uintptr_t, size_t>;

// `allocs` is a hash table mapping active pointer address to allocation size.
// `frees` is a vector of freed allocations.

// These will need to be reimplemented in C
//      static std::unordered_map<uintptr_t, size_t> allocs;
//      static std::vector<base_allocation> frees;
static int disabled;

static unsigned alloc_random() {
    static uint64_t x = 8973443640547502487ULL;
    x = x * 6364136223846793005ULL + 1ULL;
    return x >> 32;
}

static void base_allocator_atexit();

void* base_malloc(size_t sz) {
    if (disabled) {
        return malloc(sz);
    }
    ++disabled;
    uintptr_t ptr = 0;

    static int base_alloc_atexit_installed = 0;
    if (!base_alloc_atexit_installed) {
        atexit(base_allocator_atexit);
        base_alloc_atexit_installed = 1;
    }

    // try to use a previously-freed block 75% of the time
    unsigned r = alloc_random();
    if (r % 4 != 0) {
        /// Your code here. C++ uses a hash table and a vector
        // for (unsigned ntries = 0; ntries < 10 && ntries < frees.size(); ++ntries) {
        //     auto& f = frees[alloc_random() % frees.size()];
        //     if (f.second >= sz) {
        //         allocs.insert(f);
        //         ptr = f.first;
        //         f = frees.back();
        //         frees.pop_back();
        //         break;
        //     }
        // }
    }

    if (!ptr) {
        // need a new allocation
        ptr = (uintptr_t)malloc(sz ? sz : 1);
    }
    if (ptr) {
        /// Your code here. C++ uses a hash table: allocs[reinterpret_cast<uintptr_t>(ptr)] = sz;
        //allocs[ptr] = sz;
    }

    --disabled;
    return (void*)ptr;
}

void base_free(void* ptr) {
    if (disabled || !ptr) {
        free(ptr);
    } else {
        // mark free if found; if not found, invalid free: silently ignore
        ++disabled;
        /// Your code here. C++ uses a vector, you will need to do your own.
        //auto it = allocs.find((uintptr_t)ptr);
        //if (it != allocs.end()) {
            //frees.push_back(*it);
            //allocs.erase(it);
        //}
        --disabled;
    }
}

void base_allocator_disable(bool d) {
    disabled = d;
}

static void base_allocator_atexit() {
    /// Your code here
    // clean up freed memory to shut up leak detector
    //for (auto& alloc : frees) {
        //free((void*)alloc.first);
    //}
}