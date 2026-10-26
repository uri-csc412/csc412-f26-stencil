#include <stddef.h>
#include <stdint.h>

// struct for our arena
typedef struct {
    uint8_t *buff;
    size_t size;     // total size (capacity) of our arena
    size_t offset;   // current bump pointer
} Arena;

// create the arena with a pre-allocated buffer and and buffer size
void arena_init(Arena *arena, size_t buffer_size);

// allocates memory from the arena. program errors out if no more memory.
// each call to arena_alloc() returns a pointer into that block and bumps offset forward
void *arena_alloc(Arena *arena, size_t bytes); 

// resets the arena for reuse
void arena_reset(Arena *arena);

// frees all memory in the arena by resetting the offset
void arena_free(Arena *arena); 
