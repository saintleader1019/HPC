#include <stdio.h>    // printf
#include <stdint.h>   // uint64_t (enteros de 64 bits sin signo)
#include <math.h>     // sinf (seno), requiere enlazar con -lm
#include <time.h>     // clock_gettime para medir tiempo

/* ====== RNG xorshift64* y utilidades ====== */
static inline uint64_t splitmix64(uint64_t x){
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
typedef struct { uint64_t s; } xrng_t;             // estado del generador
static inline void xrng_seed(xrng_t* r, uint64_t seed){ r->s = splitmix64(seed); }
static inline uint64_t xorshift64s(xrng_t* r){     // siguiente entero pseudoaleatorio
    uint64_t x = r->s;
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    r->s = x;
    return x * 0x2545F4914F6CDD1DULL;
}
static inline double xrng_uniform01(xrng_t* r){    // real uniforme en [0,1)
    return (xorshift64s(r) >> 11) * (1.0/9007199254740992.0);
}
static inline double now_sec(void){                // cronómetro en segundos
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec*1e-9;
}
/* ========================================== */

int main(int argc, char** argv){
    // N = número de lanzamientos de la aguja (muestras).
    uint64_t N = (argc>1)? strtoull(argv[1], NULL, 10) : (uint64_t)50000000;

    // Parámetros geométricos clásicos: líneas separadas t y aguja de longitud l (l <= t).
    const float t = 1.0f;   // distancia entre líneas paralelas
    const float l = 1.0f;   // longitud de la aguja (<= t)

    xrng_t R; xrng_seed(&R, 987654321ULL);   // semilla fija (reproducible)
    uint64_t hits = 0;                       // cuántas veces la aguja cruza una línea

    double t0 = now_sec();

    for(uint64_t i = 0; i < N; i++){
        // d = distancia del centro de la aguja a la línea más cercana, uniforme en [0, t/2)
        float d = (float)xrng_uniform01(&R) * (t * 0.5f);
        // theta = ángulo de la aguja respecto a las líneas, uniforme en [0, pi/2]
        const float PI = 3.14159265358979323846f;
        float theta = (float)xrng_uniform01(&R) * (PI * 0.5f);

        // Condición de cruce: d <= (l/2) * sin(theta)
        if (d <= 0.5f * l * sinf(theta)) hits++;
    }

    double t1 = now_sec();

    // Estimación de pi:
    // p = hits / N ≈ (2*l) / (pi * t)  =>  pi ≈ (2*l) / (t * p)
    if (hits == 0){
        // Con N muy pequeño podría pasar; avisamos para evitar división por cero.
        printf("Muy pocas muestras: no hubo cruces. Aumenta N.\n");
        return 1;
    }
    double p = (double)hits / (double)N;
    double pi_est = (2.0 * (double)l) / ((double)t * p);

    printf("pi=%.9f  N=%llu  hits=%llu  time=%.3fs\n",
           pi_est, (unsigned long long)N, (unsigned long long)hits, t1 - t0);
    return 0;
}
