#include <stdio.h>
#include <stdint.h>
#include <time.h>

/* ========= RNG xorshift64* y utilidades =========
   Necesitamos números aleatorios rápidos y “por software”.
   Usaremos un generador llamado xorshift64* (simple y veloz).
*/

// Función de mezcla para crear una buena semilla a partir de un número.
static inline uint64_t splitmix64(uint64_t x){
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

// Definimos un "tipo" llamado xrng_t que guarda el estado del RNG.
typedef struct { uint64_t s; } xrng_t;

// Ponerle una semilla (seed) al RNG: inicializa su estado interno.
static inline void xrng_seed(xrng_t* r, uint64_t seed){
    r->s = splitmix64(seed);
}

// Un paso del generador xorshift64* (produce un entero aleatorio de 64 bits).
static inline uint64_t xorshift64s(xrng_t* r){
    uint64_t x = r->s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    r->s = x;
    return x * 0x2545F4914F6CDD1DULL;
}

// Convierte el entero aleatorio a un número real uniforme en [0,1).
static inline double xrng_uniform01(xrng_t* r){
    // Tomamos 53 bits aleatorios y los escalamos a un double en [0,1)
    return (xorshift64s(r) >> 11) * (1.0/9007199254740992.0);
}

// Cronómetro sencillo en segundos (alta resolución).
static inline double now_sec(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec*1e-9;
}
/* =============================================== */

int main(int argc, char** argv){
    // N es la cantidad de puntos aleatorios (muestras). Por defecto 100 millones.
    uint64_t N = (argc>1)? strtoull(argv[1], NULL, 10) : (uint64_t)1e8;

    // Creamos el RNG y lo sembramos con una semilla fija (repetible).
    xrng_t R;
    xrng_seed(&R, 123456789ULL);

    // Contador de puntos que cayeron dentro del cuarto de círculo.
    uint64_t in = 0;

    double t0 = now_sec(); // tiempo de inicio

    // Bucle principal: genera N puntos (x,y) uniformes en [0,1]x[0,1]
    for(uint64_t i=0; i<N; i++){
        double x = xrng_uniform01(&R);
        double y = xrng_uniform01(&R);
        // Si x^2 + y^2 <= 1, el punto cayó dentro del cuarto de círculo
        if (x*x + y*y <= 1.0) in++;
    }

    double t1 = now_sec(); // tiempo final

    // Proporción de puntos dentro ≈ área del cuarto de círculo (π/4)
    // Por tanto, pi ≈ 4 * in/N
    double pi = 4.0 * (double)in / (double)N;

    // Imprimimos resultado, N, cuántos cayeron dentro, y el tiempo medido
    printf("pi=%.9f  N=%llu  in=%llu  time=%.3fs\n",
           pi,
           (unsigned long long)N,
           (unsigned long long)in,
           t1 - t0);

    return 0; // Fin del programa (código 0 = OK)
}
