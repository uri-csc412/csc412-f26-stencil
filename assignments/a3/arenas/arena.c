#include "arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdalign.h>   // super important to keep our bytes algined per OS - alignof(max_align_t)
#include <stdint.h>     // this too for alignof(max_align_t

/*
* initiales our memory arena with a set size
* returns nothing or exits if our OS is out of mem
*/
void arena_init(Arena *arena, size_t buffer_size) {

}

/*
* uses our arena to grab a piece of heap memory
* returns a pointer containing the new heap memory
*/
void *arena_alloc(Arena *arena, size_t bytes) {
    // we are assuming 15-byte alignment for this example
    size_t aligned = (bytes + 15) & ~((size_t)15);

    return NULL;
}

/*
* resets arena for reuse
* returns nothing
*/
void arena_reset(Arena *arena) {
    
}

/*
* clean up arean: free and reset everything related to the arena
* returns nothing
*/
void arena_free(Arena *arena) {

}