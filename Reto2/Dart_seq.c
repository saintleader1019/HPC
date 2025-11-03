#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/* RNG xorshift64* */
static inline uint64_t splitmix64(uint64_t x){
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
typedef struct { uint64_t s; } xrng_t;
static inline void xrng_seed(xrng_t* r, uint64_t seed){ r->s = splitmix64(seed); }
static inline uint64_t xorshift64s(xrng_t* r){
    uint64_t x = r->s; x ^= x >> 12; x ^= x << 25; x ^= x >> 27; r->s = x;
    return x * 0x2545F4914F6CDD1DULL;
}
static inline double xrng_uniform01(xrng_t* r){
    return (xorshift64s(r) >> 11) * (1.0/9007199254740992.0);
}

/* timer */
static inline double now_sec(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec*1e-9;
}

int main(int argc, char** argv){
    uint64_t N = (argc > 1)? strtoull(argv[1], NULL, 10) : (uint64_t)100000000;
    const int threads = 1;  /* serial */

    xrng_t R; xrng_seed(&R, 123456789ULL);
    uint64_t in = 0;

    double t0 = now_sec();
    for(uint64_t i=0;i<N;i++){
        double x = xrng_uniform01(&R);
        double y = xrng_uniform01(&R);
        if (x*x + y*y <= 1.0) in++;
    }
    double t1 = now_sec();

    double pi = 4.0 * (double)in / (double)N;
    const char* out_path = (argc > 2)? argv[2] : NULL;
    if (out_path){
        FILE* f = fopen(out_path, "a");
        if (f){
            fprintf(f, "%llu,%d,%.9f,%.6f\n",
                    (unsigned long long)N, threads, pi, (t1 - t0));
            fclose(f);
        }
    }
    return 0;
}
