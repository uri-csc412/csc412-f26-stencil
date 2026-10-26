#include "dmalloc.h"
// Wild free.

int main() {
    int x;
    free(&x);
    print_statistics();
}

//! MEMORY BUG???: invalid free of pointer ???, not in heap
//! ???
