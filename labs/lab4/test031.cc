#include "dmalloc.h"
// Advanced error message for freeing data inside another heap block.

int main() {
    void* ptr = malloc(2001);
    free((char*) ptr + 128);
    print_statistics();
}

//! MEMORY BUG: test???.cc:6: invalid free of pointer ???, not allocated
//! test???.cc:5: ??? is 128 bytes inside a 2001 byte region allocated here
//! ???
