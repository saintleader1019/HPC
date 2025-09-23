#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "common.h"

typedef struct {
    long long a, b;     // rango [a,b)
    uint64_t seed;      // semilla por hilo
    long long local_hit;
} task_t;

static void* worker(void* arg){
    task_t* t = (task_t*)arg;
    rng64_t rng = {.s = t->seed};
    long long hit = 0;
    for (long long i = t->a; i < t->b; ++i){
        (void)i; // el índice solo divide trabajo
        double x = rand01(&rng);
        double y = rand01(&rng);
        if (x*x + y*y <= 1.0) hit++;
    }
    t->local_hit = hit;
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

    pthread_t* th   = (pthread_t*)malloc(T*sizeof(*th));
    task_t*    tasks= (task_t*)calloc(T, sizeof(*tasks));

    long long chunk = N / T, rem = N % T;

    // Preparar tareas y CREAR hilos
    for (int i=0;i<T;i++){
        long long a = i*chunk + (i<rem? i : rem);
        long long b = a + chunk + (i<rem?1:0);
        tasks[i].a = a; tasks[i].b = b;
        tasks[i].seed = 0x9e3779b97f4a7c15ULL ^ (uint64_t)(i+1);
        tasks[i].local_hit = 0;
        pthread_create(&th[i], NULL, worker, &tasks[i]);
    }

    // Medir SOLO el cómputo: desde que todos los hilos ya están creados
    double t0 = sec_now();

    long long total = 0;
    for (int i=0;i<T;i++){
        pthread_join(th[i], NULL);
        total += tasks[i].local_hit;
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
