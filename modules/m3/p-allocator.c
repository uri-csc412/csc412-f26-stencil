#include "process.h"
#include "lib.h"
#define ALLOC_SLOWDOWN 100

extern uint8_t end[];

// These global variables go on the data page.
uint8_t* heap_top;
uint8_t* stack_bottom;

void process_main(void) {
    pid_t p = sys_getpid();
    srand(p);

    // The heap starts on the page right after the 'end' symbol,
    // whose address is the first address not allocated to process code
    // or data.
    heap_top = ROUNDUP((uint8_t*) end, PAGESIZE);

    // The bottom of the stack is the first address on the current
    // stack page (this process never needs more than one stack page).
    stack_bottom = ROUNDDOWN((uint8_t*) read_rsp() - 1, PAGESIZE);

#ifdef TEST_BAD_ALLOC
    // Only built for the extra credit tests in `make grade-four`. Each bad
    // request below must return -1. Each rejected request claims a marker
    // page, which parse_log.py looks for in the memory dumps. The misaligned
    // request runs in its own boot because a kernel that doesn't reject it
    // crashes.
#if TEST_BAD_ALLOC == 1
    int heap_ready = sys_page_alloc(heap_top) == 0;
    if (sys_page_alloc((void*) 0x1000) == -1) {         // kernel memory
        sys_page_alloc((void*) 0x1F0000);
    }
    if (sys_page_alloc((void*) 0x300000) == -1) {       // past the end of VM
        sys_page_alloc((void*) 0x1F1000);
    }
    if (heap_ready && sys_page_alloc(heap_top) == -1) { // already mapped
        sys_page_alloc((void*) 0x1F2000);
    }
#else
    if (sys_page_alloc(heap_top + 1) == -1) {           // misaligned
        sys_page_alloc((void*) 0x1F3000);
    }
#endif
    while (1) {
        sys_yield();
    }
#endif

    // Allocate heap pages until (1) hit the stack (out of address space)
    // or (2) allocation fails (out of physical memory).
    while (1) {
#if !NO_SLOWDOWN
        if ((rand() % ALLOC_SLOWDOWN) < p) {
#endif
            if (heap_top == stack_bottom || sys_page_alloc(heap_top) < 0) {
                break;
            }
            *heap_top = p;      /* check we have write access to new page */
            heap_top += PAGESIZE;
#if !NO_SLOWDOWN
        }
#endif
        sys_yield();
    }

    // After running out of memory, do nothing forever
    while (1) {
        sys_yield();
    }
}
