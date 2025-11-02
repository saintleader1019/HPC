#include <stdio.h>    // printf
#include <stdint.h>   // uint64_t
#include <math.h>     // sinf (enlace con -lm)
#include <time.h>     // clock_gettime, time
#include <omp.h>      // OpenMP

/* ====== RNG xorshift64* y utilidades ====== */
static inline uint64_t splitmix64(uint64_t x){
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
typedef struct { uint64_t s; } xrng_t;
static inline void xrng_seed(xrng_t* r, uint64_t seed){ r->s = splitmix64(seed); }
static inline uint64_t xorshift64s(xrng_t* r){
    uint64_t x = r->s;
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    r->s = x;
    return x * 0x2545F4914F6CDD1DULL;
}
static inline double xrng_uniform01(xrng_t* r){
    return (xorshift64s(r) >> 11) * (1.0/9007199254740992.0); // [0,1)
}
static inline double now_sec(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec*1e-9;
}
/* ========================================== */

int main(int argc, char** argv){
    // N = número de lanzamientos (muestras)
    uint64_t N = (argc>1)? strtoull(argv[1], NULL, 10) : (uint64_t)50000000;

    // Geometría clásica: líneas separadas t y aguja de longitud l (l <= t)
    const float t = 1.0f;
    const float l = 1.0f;
    const float PI = 3.14159265358979323846f;

    double t0 = now_sec();

    // Contador global de cruces. Usaremos "reduction" para sumar por hilo.
    uint64_t hits = 0;

    #pragma omp parallel
    {
        // RNG independiente por hilo (semilla distinta)
        xrng_t R;
        uint64_t seed = 0x243F6A8885A308D3ULL
                      ^ (uint64_t)omp_get_thread_num()
                      ^ (uint64_t)(uintptr_t)&R
                      ^ (uint64_t)time(NULL);
        xrng_seed(&R, seed);

        // Cada hilo acumula sus propios hits y luego se reducen (+)
        #pragma omp for schedule(static) reduction(+:hits)
        for (uint64_t i = 0; i < N; i++){
            float d     = (float)xrng_uniform01(&R) * (t * 0.5f);     // [0, t/2)
            float theta = (float)xrng_uniform01(&R) * (PI * 0.5f);    // [0, pi/2]
            if (d <= 0.5f * l * sinf(theta)) hits++;
        }
    } // fin región paralela

    double t1 = now_sec();

    if (hits == 0){
        printf("Muy pocas muestras: no hubo cruces. Aumenta N.\n");
        return 1;
    }

    double p = (double)hits / (double)N;
    double pi_est = (2.0 * (double)l) / ((double)t * p);

    printf("pi=%.9f  N=%llu  hits=%llu  threads=%d  time=%.3fs\n",
           pi_est,
           (unsigned long long)N,
           (unsigned long long)hits,
           omp_get_max_threads(),
           t1 - t0);

    return 0;
}
