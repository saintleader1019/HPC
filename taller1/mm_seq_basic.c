// mm_seq_min.c
// Compilar:  gcc -O3 -std=c11 mm_seq_min.c -o mm_seq_min
// Ejecutar:  ./mm_seq_min 512          (N = 512)
//            ./mm_seq_min 512 12345    (N = 512, semilla = 12345)
//
// Contiene únicamente:
//  - alloc_matrix / alloc_zero_matrix
//  - random_int32 / fill_random_int32
//  - matmul_seq (O(n^3))
//  - parse_positive_int (para leer N y semilla)
// Sin E/S estándar (no printf/scanf).

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

// -------------------- Random 32-bit --------------------
static int32_t random_int32(void) {
    int32_t hi = (int32_t)(rand() & 0xFFFF);
    int32_t lo = (int32_t)(rand() & 0xFFFF);
    return (hi << 16) | lo;
}

static void fill_random_int32(int32_t *M, int n) {
    size_t nn = (size_t)n * (size_t)n;
    for (size_t i = 0; i < nn; ++i) M[i] = random_int32();
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

// -------------------- main (sin I/O estándar) --------------------
int main(int argc, char **argv) {
    if (argc < 2) return 2;        // falta N
    int N = 0;
    if (!parse_positive_int(argv[1], &N)) return 3; // N inválido

    if (argc >= 3) {
        int seed = 0;
        if (!parse_positive_int(argv[2], &seed)) return 4; // semilla inválida
        srand((unsigned)seed);
    } else {
        srand((unsigned)time(NULL));
    }

    int32_t *A = alloc_matrix(N);
    int32_t *B = alloc_matrix(N);
    int32_t *C = alloc_zero_matrix(N);
    if (!A || !B || !C) {
        free(A); free(B); free(C);
        return 6; // memoria insuficiente
    }

    fill_random_int32(A, N);
    fill_random_int32(B, N);
    matmul_seq(A, B, C, N);

    // No imprimimos ni devolvemos datos; solo liberamos y salimos
    free(A); free(B); free(C);
    return 0;
}
