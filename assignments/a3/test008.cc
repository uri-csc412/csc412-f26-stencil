#include "dmalloc.h"
// Null pointers are freeable.

int main() {
    void* p = malloc(10);
    free(nullptr);
    free(p);
    print_statistics();
}

//! alloc count: active          0   total          1   fail          0
//! alloc size:  active          0   total         10   fail          0
