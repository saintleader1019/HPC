// Compilar:
//   gcc -O3 -std=c11 -march=native -fopenmp mm_omp_time_log.c -o mm_omp_time_log
//
// Ejecutar (ejemplos):
//   ./mm_omp_time_log 1000 4 tiempos_omp.txt
//   ./mm_omp_time_log 2000 8 tiempos_omp.txt 12345
//
// Args:
//   1) N        (tamaño de la matriz cuadrada, obligatorio)
//   2) T        (número de hilos OpenMP, obligatorio)
//   3) outfile  (ruta del archivo de salida, obligatorio)
//   4) [seed]   (opcional; si no se da, usa time(NULL))
//
// Salida (append), una línea por corrida:
//   N=<N> T=<T> <segundos>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <omp.h>

static int parse_positive_int(const char *s, int *out) {
    errno = 0; char *e = NULL; long v = strtol(s, &e, 10);
    if (errno || e == s || *e != '\0' || v <= 0 || v > INT_MAX) return 0;
    *out = (int)v; return 1;
}

static int32_t* alloc_matrix(int n) {
    size_t nn = (size_t)n * (size_t)n;
    return (int32_t*)malloc(nn * sizeof(int32_t));
}
static int32_t* alloc_zero_matrix(int n) {
    size_t nn = (size_t)n * (size_t)n;
    return (int32_t*)calloc(nn, sizeof(int32_t));
}

static void fill_random_int32(int32_t *M, int n) {
    size_t nn = (size_t)n * (size_t)n;
    for (size_t i = 0; i < nn; ++i)
        M[i] = (int32_t)((rand() & 0xFFFF) - 32768); // [-32768, 32767]
}

// Kernel paralelo OpenMP (O(n^3)), sin transponer B
static void matmul_omp(const int32_t *A, const int32_t *B, int32_t *C, int n) {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            int64_t acc = 0;
            for (int k = 0; k < n; ++k) {
                acc += (int64_t)A[(size_t)i*n + (size_t)k] *
                       (int64_t)B[(size_t)k*n + (size_t)j];
            }
            C[(size_t)i*n + (size_t)j] = (int32_t)acc;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 4) return 2;

    int N = 0, T = 0;
    if (!parse_positive_int(argv[1], &N)) return 3;
    if (!parse_positive_int(argv[2], &T)) return 3;
    const char *outfile = argv[3];

    if (argc >= 5) {
        int seed = 0;
        if (!parse_positive_int(argv[4], &seed)) return 4;
        srand((unsigned)seed);
    } else {
        srand((unsigned)time(NULL));
    }

    omp_set_num_threads(T);

    int32_t *A = alloc_matrix(N);
    int32_t *B = alloc_matrix(N);
    int32_t *C = alloc_zero_matrix(N);
    if (!A || !B || !C) {
        free(A); free(B); free(C);
        return 6;
    }

    fill_random_int32(A, N);
    fill_random_int32(B, N);

    // Medimos SOLO la multiplicación
    double t0 = omp_get_wtime();
    matmul_omp(A, B, C, N);
    double t1 = omp_get_wtime();
    double elapsed = t1 - t0;

    // Guardar tiempo en archivo
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
