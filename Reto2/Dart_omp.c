#include <stdio.h>    // printf
#include <stdint.h>   // uint64_t
#include <time.h>     // clock_gettime, time
#include <omp.h>      // OpenMP

/* ======= RNG xorshift64* y utilidades (idéntico enfoque al serial) ======= */
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
    x ^= x >> 12; 
    x ^= x << 25; 
    x ^= x >> 27; 
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
/* ======================================================================== */

int main(int argc, char** argv){
    // N = número de puntos aleatorios a generar.
    uint64_t N = (argc>1)? strtoull(argv[1], NULL, 10) : (uint64_t)1e8;

    // Contador global de puntos "dentro" del cuarto de círculo.
    // OJO: lo actualizaremos con cuidado para evitar condiciones de carrera.
    uint64_t in = 0;

    double t0 = now_sec();

    // Región paralela: a partir de aquí, varios hilos ejecutan el bloque.
    #pragma omp parallel
    {
        // Cada hilo tiene su propio generador RNG con su propia semilla.
        xrng_t R;
        uint64_t seed = 0x9e3779b97f4a7c15ULL
                      ^ (uint64_t)omp_get_thread_num()     // id del hilo
                      ^ (uint64_t)(uintptr_t)&R            // dirección (casi-única)
                      ^ (uint64_t)time(NULL);              // tiempo actual
        xrng_seed(&R, seed);

        // Contador local del hilo: evita "pelea" entre hilos por la misma variable.
        uint64_t local_in = 0;

        // Paralelizamos el bucle for. Cada hilo procesa un "trozo" de las N iteraciones.
        #pragma omp for schedule(static) nowait
        for (uint64_t i=0; i<N; i++){
            double x = xrng_uniform01(&R);
            double y = xrng_uniform01(&R);
            if (x*x + y*y <= 1.0) local_in++;
        }

        // Al final, cada hilo suma su resultado local al contador global de forma atómica.
        #pragma omp atomic
        in += local_in;
    } // Fin de la región paralela

    double t1 = now_sec();

    double pi = 4.0 * (double)in / (double)N;
    printf("pi=%.9f  N=%llu  in=%llu  threads=%d  time=%.3fs\n",
           pi,
           (unsigned long long)N,
           (unsigned long long)in,
           omp_get_max_threads(),   // número de hilos disponibles
           t1 - t0);

    return 0;
}
