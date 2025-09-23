#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include "common.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    long long a, b;     // rango [a,b)
    uint64_t seed;      // semilla por hilo
    double L, D;        // parámetros de Buffon (aquí L=D=1.0)
    long long local_cross;
} task_t;

static void* worker(void* arg){
    task_t* t = (task_t*)arg;
    rng64_t rng = {.s = t->seed};
    long long cross = 0;
    for (long long i = t->a; i < t->b; ++i){
        (void)i;
        double theta = M_PI * rand01(&rng);   // θ ~ U[0,π)
        double y     = (t->D * 0.5) * rand01(&rng); // y ~ U[0,D/2)
        if (0.5 * t->L * fabs(sin(theta)) >= y) cross++;
    }
    t->local_cross = cross;
    return NULL;
}

int main(int argc, char** argv){
    if (argc < 4){
        fprintf(stderr, "Uso: %s N T outfile\n", argv[0]);
        return 1;
    }
    long long N = atoll(argv[1]);
    int T = atoi(argv[2]);
    const char* outfile = argv[3];
    if (T <= 0) T = 1;

    // Parámetros clásicos
    const double L = 1.0, D = 1.0;

    pthread_t* th   = (pthread_t*)malloc(T*sizeof(*th));
    task_t*    tasks= (task_t*)calloc(T, sizeof(*tasks));

    long long chunk = N / T, rem = N % T;

    // Preparar tareas y CREAR hilos
    for (int i=0;i<T;i++){
        long long a = i*chunk + (i<rem? i : rem);
        long long b = a + chunk + (i<rem?1:0);
        tasks[i].a = a; tasks[i].b = b;
        tasks[i].seed = 0x243f6a8885a308d3ULL ^ (uint64_t)(i+1);
        tasks[i].L = L; tasks[i].D = D;
        tasks[i].local_cross = 0;
        pthread_create(&th[i], NULL, worker, &tasks[i]);
    }

    // Medición SOLO del cómputo
    double t0 = sec_now();

    long long total = 0;
    for (int i=0;i<T;i++){
        pthread_join(th[i], NULL);
        total += tasks[i].local_cross;
    }

    double t1 = sec_now();
    double elapsed = t1 - t0;

    FILE *f = fopen(outfile, "a");
    if (f) {
        fprintf(f, "N=%lld %.6f\n", N, elapsed);
        fclose(f);
    } else {
        perror("fopen");
        free(th); free(tasks);
        return 2;
    }

    free(th); free(tasks);
    return 0;
}
