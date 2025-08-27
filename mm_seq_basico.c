// mm_seq_basico.c
// Compilar:  gcc -O3 -std=c11 mm_seq_basico.c -o mm_seq_basico
// Ejecutar:  ./mm_seq_basico 512         (N = 512)
//            ./mm_seq_basico 512 12345   (N = 512, semilla = 12345)

#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

static volatile uint32_t g_sink = 0; // evita que el compilador elimine el cálculo

// --- Generación de enteros aleatorios de 32 bits ---
static int32_t random_int32(void) {
    int32_t hi = (int32_t)(rand() & 0xFFFF);
    int32_t lo = (int32_t)(rand() & 0xFFFF);
    return (hi << 16) | lo;
}

// --- Utilidades ---
static int parse_positive_int(const char *s, int *out) {
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return 0;
    if (v <= 0 || v > INT_MAX) return 0;
     *out = (int)v;
    return 1;
}

static int32_t* crear_matriz(int n) {
    size_t nn = (size_t)n * (size_t)n;
    int32_t *M = (int32_t*)malloc(nn * sizeof(int32_t));
    if (!M) return NULL;
    for (size_t i = 0; i < nn; ++i) M[i] = random_int32();
    return M;
}

static int32_t* crear_matriz_cero(int n) {
    size_t nn = (size_t)n * (size_t)n;
    int32_t *M = (int32_t*)calloc(nn, sizeof(int32_t));
    return M;
}

// --- Multiplicación secuencial: C = A * B ---
static void matmul_seq(const int32_t *A, const int32_t *B, int32_t *C, int n) {
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

// --- Checksum simple para "usar" C y evitar DCE ---
static uint32_t checksum32(const int32_t *C, int n) {
    uint32_t s = 0;
    size_t nn = (size_t)n * (size_t)n;
    for (size_t i = 0; i < nn; ++i) {
        s ^= (uint32_t)C[i] + 0x9e3779b9u + (s << 6) + (s >> 2);
    }
    return s;
}

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    int N = 0;
    if (!parse_positive_int(argv[1], &N)) return 3;

    if (argc >= 3) {
        int seed = 0;
        if (!parse_positive_int(argv[2], &seed)) return 4;
        srand((unsigned)seed);
    } else {
        srand((unsigned)time(NULL));
    }

    int32_t *A = crear_matriz(N);
    int32_t *B = crear_matriz(N);
    int32_t *C = crear_matriz_cero(N);
    if (!A || !B || !C) {
        free(A); free(B); free(C);
        return 6;
    }

    matmul_seq(A, B, C, N);

    uint32_t s = checksum32(C, N);
    g_sink = s;

    free(A);
    free(B);
    free(C);

    return (int)(s & 0xFF);
}
