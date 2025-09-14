// mm_mt_time_log.c (v2 sin transposición)
// Compilar:  gcc -O3 -std=c11 -march=native -pthread mm_threads.c -o mm_threads_time_log
// Ejecutar (ejemplos):
//   ./mm_threads_time_log 1000 4 tiempos.txt
//   ./mm_threads_time_log 2000 8 12345 tiempos.txt
//
// Argumentos:
//   1) N              (tamaño de la matriz cuadrada, obligatorio)
//   2) T              (número de hilos, obligatorio)
//   3) [seed]         (opcional; si no se da, usa time(NULL))
//   4) outfile        (ruta del archivo de salida, obligatorio si hay seed; si no hay seed, es el 3er arg)
//
// Salida a archivo (append), una línea por corrida: "N=<N> T=<T> <segundos>"

#define _POSIX_C_SOURCE 199309L
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>      // solo para E/S a archivo (no stdout/stderr)
//sudo apt update
//sudo apt install build-essential
#include <pthread.h>

typedef struct {
    const int32_t *A;
    const int32_t *B;   // SIEMPRE B original (no transpuesta)
    int32_t *C;
    int n;
    int t_id;
    int t_count;
} worker_args_t;

// -------------------- Utilidades --------------------
static int parse_positive_int(const char *s, int *out) {
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return 0;
    if (v <= 0 || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

static int32_t* alloc_matrix(int n) {
    size_t nn = (size_t)n * (size_t)n;
    return (int32_t*)malloc(nn * sizeof(int32_t));
}

static int32_t* alloc_zero_matrix(int n) {
    size_t nn = (size_t)n * (size_t)n;
    return (int32_t*)calloc(nn, sizeof(int32_t));
}

static int32_t random_int32(void) {
    int32_t hi = (int32_t)(rand() & 0xFFFF);
    int32_t lo = (int32_t)(rand() & 0xFFFF);
    return (hi << 16) | lo;
}

static void fill_random_int32(int32_t *M, int n) {
    size_t nn = (size_t)n * (size_t)n;
    for (size_t i = 0; i < nn; ++i) {
        // Para reducir riesgo de overflow acumulado extremo, acotamos a 16 bits.
        M[i] = (int32_t)((rand() & 0xFFFF) - 32768); // [-32768, 32767]
        // Si deseas usar todo 32-bit: M[i] = random_int32();
    }
}

// -------------------- Worker (O(n^3) paralelizado por filas) --------------------
static void* worker_run(void *arg) {
    worker_args_t *w = (worker_args_t*)arg;
    const int32_t *A = w->A;
    const int32_t *B = w->B;   // B ORIGINAL
    int32_t *C = w->C;
    int n = w->n;
    int tid = w->t_id;
    int tcount = w->t_count;

    // Particiona por filas de C: cada hilo procesa un bloque contiguo.
    int rows_per_thread = n / tcount;
    int rem = n % tcount;
    int start = tid * rows_per_thread + (tid < rem ? tid : rem);
    int count = rows_per_thread + (tid < rem ? 1 : 0);
    int end = start + count;

    // Versión directa: C[i,j] = sum_k A[i,k] * B[k,j]
    for (int i = start; i < end; ++i) {
        for (int j = 0; j < n; ++j) {
            int64_t acc = 0;
            for (int k = 0; k < n; ++k) {
                acc += (int64_t)A[(size_t)i*n + (size_t)k] *
                       (int64_t)B[(size_t)k*n + (size_t)j];
            }
            C[(size_t)i*n + (size_t)j] = (int32_t)acc; // recorta a 32 bits
        }
    }
    return NULL;
}

// -------------------- Multiplicación paralela con T hilos --------------------
static void matmul_parallel(const int32_t *A, const int32_t *B, int32_t *C, int n, int T) {
    if (T < 1) T = 1;
    if (T > n) T = n; // no más hilos que filas

    pthread_t *threads = (pthread_t*)malloc((size_t)T * sizeof(pthread_t));
    worker_args_t *args = (worker_args_t*)malloc((size_t)T * sizeof(worker_args_t));

    for (int t = 0; t < T; ++t) {
        args[t].A = A;
        args[t].B = B;   // B original
        args[t].C = C;
        args[t].n = n;
        args[t].t_id = t;
        args[t].t_count = T;
        pthread_create(&threads[t], NULL, worker_run, &args[t]);
    }
    for (int t = 0; t < T; ++t) {
        pthread_join(threads[t], NULL);
    }

    free(args);
    free(threads);
}

// -------------------- main --------------------
int main(int argc, char **argv) {
    // Esperado:
    //   argv[1] = N            (obligatorio)
    //   argv[2] = T            (#hilos, obligatorio)
    //   argv[3] = [seed]       (opcional)
    //   argv[4] = outfile      (si hay seed)  |  argv[3] = outfile (si no hay seed)
    if (argc < 3) return 2;

    int N = 0, T = 0;
    if (!parse_positive_int(argv[1], &N)) return 3;
    if (!parse_positive_int(argv[2], &T)) return 3;

    const char *outfile = NULL;
    if (argc == 3) {
        // Falta archivo → no sabemos dónde registrar
        return 5;
    } else if (argc == 4) {
        // Sin semilla; argv[3] = outfile
        srand((unsigned)time(NULL));
        outfile = argv[3];
    } else {
        // Con semilla y outfile
        int seed = 0;
        if (!parse_positive_int(argv[3], &seed)) return 4;
        srand((unsigned)seed);
        outfile = argv[4];
    }

    int32_t *A = alloc_matrix(N);
    int32_t *B = alloc_matrix(N);
    int32_t *C = alloc_zero_matrix(N);
    if (!A || !B || !C) {
        free(A); free(B); free(C);
        return 6;
    }

    fill_random_int32(A, N);
    fill_random_int32(B, N);

    // Cronometrar SOLO la multiplicación paralela (con B original)
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    matmul_parallel(A, B, C, N, T);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double elapsed = (t1.tv_sec - t0.tv_sec) +
                     (t1.tv_nsec - t0.tv_nsec) / 1e9;

    // Escribir en archivo en modo append: "N=<N> T=<T> <segundos>"
    FILE *f = fopen(outfile, "a");
    if (!f) {
        free(A); free(B); free(C);
        return 7;
    }
    fprintf(f, "N=%d T=%d %.6f\n", N, T, elapsed);
    fclose(f);

    free(A); free(B); free(C);
    return 0;
}
