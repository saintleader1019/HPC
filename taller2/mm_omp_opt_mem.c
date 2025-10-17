// Args:
//   1) N        (tamaño de la matriz cuadrada, obligatorio)
//   2) T        (número de hilos OpenMP, obligatorio)
//   3) outfile  (ruta del archivo de salida, obligatorio)
//   4) [seed]   (opcional; si no se da, usa time(NULL))
//
// Salida (append), una línea por corrida:
//   N=<N> T=<T> K=Bt <segundos>
//
// Nota: El tiempo medido **excluye** la transposición de B. Solo mide la multiplicación A × B utilizando Bt.

#define _POSIX_C_SOURCE 200112L
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <omp.h>
#include <string.h>
#include <stdalign.h>

static int parse_positive_int(const char *s, int *out) {
    errno = 0; char *e = NULL; long v = strtol(s, &e, 10);
    if (errno || e == s || *e != '\0' || v <= 0 || v > INT_MAX) return 0;
    *out = (int)v; return 1;
}

// ---- reserva alineada a 64B (posix_memalign) ----
static void* aligned_malloc64(size_t bytes) {
    void *p = NULL;
    if (posix_memalign(&p, 64, bytes) != 0) return NULL;
    return p;
}

// ---- matrices alineadas ----
static int32_t* alloc_matrix(int n) {
    size_t bytes = (size_t)n * (size_t)n * sizeof(int32_t);
    return (int32_t*)aligned_malloc64(bytes);
}
static int32_t* alloc_zero_matrix(int n) {
    int32_t *p = alloc_matrix(n);
    if (p) memset(p, 0, (size_t)n*(size_t)n*sizeof(int32_t));
    return p;
}

// ---- relleno aleatorio (acotado a 16 bits para reducir overflow acumulado) ----
static void fill_random_int32(int32_t *M, int n) {
    size_t nn = (size_t)n * (size_t)n;
    for (size_t i = 0; i < nn; ++i)
        M[i] = (int32_t)((rand() & 0xFFFF) - 32768); // [-32768, 32767]
}

// ---- transposición de B a Bt (paralela), fuera del tiempo medido ----
static void transpose_omp(const int32_t * __restrict B0,
                          int32_t       * __restrict Bt0, int n)
{
    // Afirmar alineación para vectorizador (solo si realmente están alineados)
    const int32_t *B  = __builtin_assume_aligned(B0,  64);
    int32_t       *Bt = __builtin_assume_aligned(Bt0, 64);

    #pragma omp parallel for collapse(2) schedule(static)
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            Bt[(size_t)j*n + (size_t)i] = B[(size_t)i*n + (size_t)j];
}

// ---- kernel OpenMP usando Bt (accesos contiguos en el bucle interno) ----
static void matmul_omp_with_Bt(const int32_t * __restrict A0,
                               const int32_t * __restrict Bt0,
                               int32_t       * __restrict C0, int n)
{
    const int32_t *A  = __builtin_assume_aligned(A0,  64);
    const int32_t *Bt = __builtin_assume_aligned(Bt0, 64);
    int32_t       *C  = __builtin_assume_aligned(C0,  64);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i) {
        const int32_t *Ai = &A[(size_t)i*n];
        for (int j = 0; j < n; ++j) {
            const int32_t *Btj = &Bt[(size_t)j*n];
            int64_t acc = 0;
            // innermost vectorizable, recorrido lineal en Ai[] y Btj[]
            for (int k = 0; k < n; ++k) {
                acc += (int64_t)Ai[k] * (int64_t)Btj[k];
            }
            C[(size_t)i*n + (size_t)j] = (int32_t)acc; // recorte a 32 bits
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

    // Reservas alineadas
    int32_t *A  = alloc_matrix(N);
    int32_t *B  = alloc_matrix(N);
    int32_t *Bt = alloc_matrix(N);
    int32_t *C  = alloc_zero_matrix(N);
    if (!A || !B || !Bt || !C) {
        free(A); free(B); free(Bt); free(C);
        return 6;
    }

    // Inicialización (first-touch paralelo opcional para NUMA)
    fill_random_int32(A, N);
    fill_random_int32(B, N);

    // Transponer B -> Bt (PARALELO) — NO se cronometra
    transpose_omp(B, Bt, N);

    // Medimos SOLO la multiplicación usando Bt
    double t0 = omp_get_wtime();
    matmul_omp_with_Bt(A, Bt, C, N);
    double t1 = omp_get_wtime();
    double elapsed = t1 - t0;

    // Guardar tiempo en archivo
    FILE *f = fopen(outfile, "a");
    if (!f) {
        free(A); free(B); free(Bt); free(C);
        return 7;
    }
    fprintf(f, "N=%d T=%d K=Bt %.6f\n", N, T, elapsed);
    fclose(f);

    free(A); free(B); free(Bt); free(C);
    return 0;
}