#include "arena.h"
#include <stdio.h>
#include <string.h> // for strlen / memcpy

static void use_our_arena(Arena *arena) {
    // get memory from the arena for 10 ints
    int *myint = arena_alloc(arena, 10 * sizeof *myint);
    for (int i = 0; i < 10; i++) myint[i] = i;

    // get memory from the arena for a C string
    const char *text = "hello world";
    size_t len = strlen(text);
    char *msg = arena_alloc(arena, len + 1);    // +1 for the null-terminator ;)
    memcpy(msg, text, len);
    msg[len] = '\0';

    // print our data
    printf("%d %s\n", myint[9], msg); // 9 hello world
}

/*
 * Just a toy example of using our arena
 */
int main(void) {
    Arena arena;
    arena_init(&arena, 1024);

    use_our_arena(&arena);

    // let's reuse the same buffer after resetting it
    // reset is faster that allocating an entire new arena
    arena_reset(&arena);
    use_our_arena(&arena);

    printf("We should see the same result twice ;)");

    arena_free(&arena);
    return 0;
}
