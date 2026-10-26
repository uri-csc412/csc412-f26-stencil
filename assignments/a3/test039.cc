#include "magic.hh"
// Wild free inside heap-allocated data.

struct whatever {
    int first[100];
    char second[3000];
    int third[200];
};

int main(int argc, char** argv) {
    dbg_allocator<whatever> allocator;
    // “allocate space for one `whatever` object”
    whatever* object = allocator.allocate(1);

    uintptr_t addr = (uintptr_t)object;
    if (argc < 2) {
        addr += 3000;
    } else {
        addr += strtol(argv[1], nullptr, 0);
    }

    whatever* trick = (whatever*)addr;
    allocator.deallocate(trick, 1);
    print_statistics();
}

//! MEMORY BUG???: invalid free of pointer ???, not allocated
//! ???
