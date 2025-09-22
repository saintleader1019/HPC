#pragma once
#include <stdint.h>
#include <time.h>

typedef struct { uint64_t s; } rng64_t;

static inline uint64_t xorshift64s(rng64_t* st) {
    uint64_t x = st->s;
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    st->s = x;
    return x * 2685821657736338717ULL;
}

static inline double rand01(rng64_t* st) {
    return (xorshift64s(st) >> 11) * (1.0/9007199254740992.0);
}

static inline double sec_now(){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec*1e-9;
}
