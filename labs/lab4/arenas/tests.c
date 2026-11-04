// Compile: make (see Makefile below)
// Run:     ./arena_tests

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdalign.h>
#include <inttypes.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "arena.h"

// Counters for keeping track of tests run and results
static int tests_run = 0;
static int tests_correct = 0;
static int tests_incorrect = 0;

// Some fun colors for testing
#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define RESET "\033[0m"

// =========================
// Tiny assertion helpers
// =========================
static void pass(const char *name) {
    tests_run++; tests_correct++;
    printf(GREEN "[PASSED %d] %s\n" RESET, tests_run, name);
}

static void fail(const char *name, const char *fmt, ...) {
    tests_run++; tests_incorrect++;
    printf(RED "[FAILED %d] %s: ", tests_run, name);
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
    printf("\n" RESET);
}

/* =========================
 * Assertion Helpers New :)
 * ========================= */

#define ASSERT_TRUE(name, cond, ...) \
    do { if ((cond)) pass(name); else fail(name, __VA_ARGS__); } while (0)

#define ASSERT_EQ_SIZE(name, a, b) \
    ASSERT_TRUE(name, (a) == (b), "expected %zu == %zu", (size_t)(a), (size_t)(b))

#define ASSERT_NONNULL(name, p) \
    ASSERT_TRUE(name, (p) != NULL, "expected non-NULL pointer")

#define ASSERT_ALIGNED(name, p, align) \
    ASSERT_TRUE(name, ((uintptr_t)(p) % (align)) == 0, \
        "pointer %p not aligned to %zu (mod=%" PRIuPTR ")", \
        (void*)(p), (size_t)(align), (uintptr_t)(p) % (size_t)(align))

#define ASSERT_MEM_EQ(name, a, b, n) \
    ASSERT_TRUE(name, memcmp((a),(b),(n)) == 0, "memory mismatch over %zu bytes", (size_t)(n))

// =========================
// Tests
// =========================
static void test_init_and_free(void) {
    Arena a;
    arena_init(&a, 1024);
    ASSERT_NONNULL("arena_init: buff non-null", a.buff);
    ASSERT_EQ_SIZE("arena_init: size set", a.size, 1024);
    ASSERT_EQ_SIZE("arena_init: offset zero", a.offset, 0);

    arena_free(&a);
    ASSERT_TRUE("arena_free: buff NULL", a.buff == NULL, "buff should be NULL");
    ASSERT_EQ_SIZE("arena_free: size zero", a.size, 0);
    ASSERT_EQ_SIZE("arena_free: offset zero", a.offset, 0);
}

static void test_basic_alloc_and_write(void) {
    Arena a;
    arena_init(&a, 1024);

    int *ints = arena_alloc(&a, 10 * sizeof *ints);
    ASSERT_NONNULL("arena_init: non-null", ints);

    for (int i = 0; i < 10; i++) ints[i] = i * 7;
    int gold[10]; for (int i = 0; i < 10; i++) gold[i] = i * 7;
    ASSERT_MEM_EQ("write/read ints", ints, gold, sizeof gold);

    arena_free(&a);
}

static void test_alignment(void) {
    Arena a;
    arena_init(&a, 256);

    const size_t A = alignof(max_align_t);

    void *p1 = arena_alloc(&a, 1);
    ASSERT_NONNULL("arena alignment: p1", p1);
    ASSERT_ALIGNED("arena alignment: p1 aligned", p1, A);

    void *p2 = arena_alloc(&a, 3);
    ASSERT_NONNULL("arena alignment: p2", p2);
    ASSERT_ALIGNED("arena alignment: p2 aligned", p2, A);

    ASSERT_TRUE("arena alignment: monotonic bump", (uintptr_t)p2 > (uintptr_t)p1,
                "expected p2 > p1");

    arena_free(&a);
}

static void test_reset_reuse(void) {
    Arena a;
    arena_init(&a, 128);

    void *p1 = arena_alloc(&a, 24);
    ASSERT_NONNULL("reset: pointer 1 (p1) non-null", p1);
    size_t first_off = a.offset;

    arena_reset(&a);
    ASSERT_EQ_SIZE("reset: offset zero", a.offset, 0);

    void *p2 = arena_alloc(&a, 24);
    ASSERT_NONNULL("reset: pointer 1 (p2) non-null", p2);
    ASSERT_TRUE("reset: buffer start reused", p2 == p1,
                "expected p2 (%p) == p1 (%p)", p2, p1);
    ASSERT_EQ_SIZE("reset: same bump", a.offset, first_off);

    arena_free(&a);
}

static void test_exact_fit_and_overflow(void) {
    Arena a;
    arena_init(&a, 64);

    // exact fit: 2 allocations of 32 (assuming alignment ≤ 16; with max_align_t it's fine here)
    void *p1 = arena_alloc(&a, 32);
    ASSERT_NONNULL("arena_alloc: exact-fit p1 32 bytes, arena is 64 bytes", p1);
    void *p2 = arena_alloc(&a, 32);
    ASSERT_NONNULL("arena_alloc: exact-fit p2 32 bytes, arena is 64 bytes", p2);
    ASSERT_EQ_SIZE("arena: offset==size", a.offset, a.size);

    // overflow behavior (your arena_alloc exits(1) on overflow).
    pid_t pid = fork();
    if (pid == 0) {
        (void)arena_alloc(&a, 1); // should trigger exit(1)
        _exit(0);                 // if not exited, fail child
    }
    int st = 0; waitpid(pid, &st, 0);
    ASSERT_TRUE("overflow: child exited non-zero",
                (WIFEXITED(st) && WEXITSTATUS(st) != 0) || WIFSIGNALED(st),
                "expected non-zero or signal; status=%d", st);

    arena_free(&a);
}

/* =========================
 * Main
 * ========================= */
static void print_summary(void) {
    printf("\nRan %d tests: %d passed, %d failed\n",
           tests_run, tests_correct, tests_incorrect);
    if (tests_incorrect == 0) {
        printf(GREEN "All tests passed.\n" RESET);
    } else {
        printf(RED "Some tests failed.\n" RESET);
    }
}

int main(void) {
    printf("Running arena tests (Linux)...\n");
    test_init_and_free();
    test_basic_alloc_and_write();
    test_alignment();
    test_reset_reuse();
    test_exact_fit_and_overflow();
    print_summary();
    return tests_incorrect == 0 ? 0 : 1;
}
