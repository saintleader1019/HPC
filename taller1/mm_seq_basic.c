// mm_seq_time_log.c
// Compilar:  gcc -O3 -std=c11 -march=native mm_seq_time_log.c -o mm_seq_time_log
// Ejecutar (con semilla opcional):
//   ./mm_seq_time_log 500 12345 tiempos.txt
//   ./mm_seq_time_log 500 tiempos.txt        // sin semilla

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>   // solo para E/S a archivo (no stdout/stderr)

// -------------------- Random 32-bit --------------------
static int32_t random_int32(void) {
    int32_t hi = (int32_t)(rand() & 0xFFFF);
    int32_t lo = (int32_t)(rand() & 0xFFFF);
    return (hi << 16) | lo;
}

static void fill_random_int32(int32_t *M, int n) {
    size_t nn = (size_t)n * (size_t)n;
    for (size_t i = 0; i < nn; ++i) {
        // Limito valores a 16 bits para reducir riesgo de overflow acumulado
        M[i] = (int32_t)((rand() & 0xFFFF) - 32768); // [-32768, 32767]
        // Si prefieres todo el rango 32-bit, usa: M[i] = random_int32();
    }
}

// -------------------- Matrices --------------------
static int32_t* alloc_matrix(int n) {
    size_t nn = (size_t)n * (size_t)n;
    return (int32_t*)malloc(nn * sizeof(int32_t));
}

static int32_t* alloc_zero_matrix(int n) {
    size_t nn = (size_t)n * (size_t)n;
    return (int32_t*)calloc(nn, sizeof(int32_t));
}

// -------------------- Multiplicación O(n^3) --------------------
static void matmul_seq(const int32_t *A, const int32_t *B, int32_t *C, int n) {
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            int64_t acc = 0;
            for (int k = 0; k < n; ++k) {
                acc += (int64_t)A[(size_t)i*n + (size_t)k] *
                       (int64_t)B[(size_t)k*n + (size_t)j];
            }
            C[(size_t)i*n + (size_t)j] = (int32_t)acc; // recorta a 32 bits
        }
    }
}

// -------------------- Utilidad CLI --------------------
static int parse_positive_int(const char *s, int *out) {
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return 0;
    if (v <= 0 || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

// -------------------- main --------------------
int main(int argc, char **argv) {
    // Argumentos esperados:
    //  - N y archivo (2 args), o
    //  - N, semilla y archivo (3 args)
    if (argc < 3) return 2;

    int N = 0;
    if (!parse_positive_int(argv[1], &N)) return 3;

    const char *outfile = NULL;
    if (argc == 3) {
        // Sin semilla: argv[2] es el archivo
        outfile = argv[2];
        srand((unsigned)time(NULL));
    } else {
        // Con semilla y archivo
        int seed = 0;
        if (!parse_positive_int(argv[2], &seed)) return 4;
        srand((unsigned)seed);
        outfile = argv[3];
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

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    matmul_seq(A, B, C, N);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    // Tiempo en segundos (double)
    double elapsed = (t1.tv_sec - t0.tv_sec) +
                     (t1.tv_nsec - t0.tv_nsec) / 1e9;

    // Abrir archivo en modo append y escribir "N=XXXX <segundos>"
    FILE *f = fopen(outfile, "tiempos.txt");
    if (f) {
        // una línea por corrida, etiquetada con N
        // Ejemplo: "N=500 0.023451"
        fprintf(f, "N=%d %.6f\n", N, elapsed);
        fclose(f);
    } else {
        // Si no se pudo abrir, retornamos error diferente
        free(A); free(B); free(C);
        return 7;
    }

    free(A); free(B); free(C);
    return 0;
}
