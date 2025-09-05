// mm_mp_time_log.c  —  v3 por procesos, SIN transposición de B
//
// Compilar (Linux):
//   gcc -O3 -std=c11 -march=native mm_mp_time_log.c -o mm_mp_time_log
// (si tu sistema lo requiere, añade -lrt para clock_gettime)
//
// Ejecutar ejemplos:
//   ./mm_mp_time_log 1000 4 tiempos_mp.txt
//   ./mm_mp_time_log 2000 8 12345 tiempos_mp.txt
//
// Argumentos:
//   1) N        (tamaño de la matriz cuadrada, obligatorio)
//   2) P        (número de procesos, obligatorio)
//   3) [seed]   (opcional; si no se da, usa time(NULL))
//   4) outfile  (si hay seed es argv[4]; si no hay seed es argv[3])
//
// Salida en archivo (append), una línea por corrida:
//   N=<N> P=<P> <segundos>

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>      // solo E/S a archivo (no stdout/stderr)
#include <sys/mman.h>   // mmap, munmap
#include <sys/wait.h>   // waitpid
#include <unistd.h>     // fork, _exit
#include <string.h>

// -------- Utilidades básicas --------
static int parse_positive_int(const char *s, int *out) {
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') return 0;
    if (v <= 0 || v > INT_MAX) return 0;
    *out = (int)v;
    return 1;
}

static void* shm_alloc(size_t bytes, int zero) {
    void *p = mmap(NULL, bytes, PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (p == MAP_FAILED) return NULL;
    if (zero) memset(p, 0, bytes);
    return p;
}
static void shm_free(void *p, size_t bytes) {
    if (p && p != MAP_FAILED) munmap(p, bytes);
}

// -------- Aleatorios 32-bit (acotados a 16-bit para evitar desbordes grandes) --------
static int32_t random_int32(void) {
    int32_t hi = (int32_t)(rand() & 0xFFFF);
    int32_t lo = (int32_t)(rand() & 0xFFFF);
    return (hi << 16) | lo;
}
static void fill_random_int32(int32_t *M, int n) {
    size_t nn = (size_t)n * (size_t)n;
    for (size_t i = 0; i < nn; ++i) {
        // Rango “seguro” de 16 bits para reducir overflow acumulado:
        M[i] = (int32_t)((rand() & 0xFFFF) - 32768); // [-32768, 32767]
        // Si quieres rango completo 32-bit, usa: M[i] = random_int32();
    }
}

// -------- Cálculo de bloque (SIN transponer B) --------
// Cada proceso calcula filas [start, end) de C: C[i,j] = sum_k A[i,k] * B[k,j]
static void matmul_block(const int32_t *A, const int32_t *B, int32_t *C,
                         int n, int start, int end)
{
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
}

// -------- Multiplicación paralela por procesos (SIN transponer B) --------
static int matmul_processes(const int32_t *A, const int32_t *B, int32_t *C, int n, int P)
{
    if (P < 1) P = 1;
    if (P > n) P = n; // no más procesos que filas

    pid_t *pids = (pid_t*)malloc((size_t)P * sizeof(pid_t));
    if (!pids) return 0;

    int rows_per = n / P;
    int rem = n % P;

    for (int p = 0; p < P; ++p) {
        int start = p * rows_per + (p < rem ? p : rem);
        int count = rows_per + (p < rem ? 1 : 0);
        int end   = start + count;

        pid_t pid = fork();
        if (pid < 0) {
            // fallo fork: espera lo ya creado y aborta
            for (int q = 0; q < p; ++q) waitpid(pids[q], NULL, 0);
            free(pids);
            return 0;
        }
        if (pid == 0) {
            // Hijo: calcula su bloque y sale
            matmul_block(A, B, C, n, start, end);
            _exit(0);
        }
        pids[p] = pid;
    }

    // Padre: espera a todos
    for (int p = 0; p < P; ++p) waitpid(pids[p], NULL, 0);

    free(pids);
    return 1;
}

// -------- main --------
int main(int argc, char **argv) {
    if (argc < 3) return 2;

    int N = 0, P = 0;
    if (!parse_positive_int(argv[1], &N)) return 3;
    if (!parse_positive_int(argv[2], &P)) return 3;

    const char *outfile = NULL;
    if (argc == 3) {
        // falta archivo
        return 5;
    } else if (argc == 4) {
        // sin semilla; argv[3] = outfile
        srand((unsigned)time(NULL));
        outfile = argv[3];
    } else {
        // con semilla y outfile
        int seed = 0;
        if (!parse_positive_int(argv[3], &seed)) return 4;
        srand((unsigned)seed);
        outfile = argv[4];
    }

    size_t bytes = (size_t)N * (size_t)N * sizeof(int32_t);

    // A, B y C en memoria compartida
    int32_t *A = (int32_t*)shm_alloc(bytes, 0);
    int32_t *B = (int32_t*)shm_alloc(bytes, 0);
    int32_t *C = (int32_t*)shm_alloc(bytes, 1); // en cero
    if (!A || !B || !C) {
        shm_free(A, bytes); shm_free(B, bytes); shm_free(C, bytes);
        return 6;
    }

    fill_random_int32(A, N);
    fill_random_int32(B, N);

    // Cronometrar SOLO la multiplicación paralela por procesos (B original)
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    int ok = matmul_processes(A, B, C, N, P);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    if (!ok) {
        shm_free(A, bytes); shm_free(B, bytes); shm_free(C, bytes);
        return 8;
    }

    double elapsed = (t1.tv_sec - t0.tv_sec) +
                     (t1.tv_nsec - t0.tv_nsec) / 1e9;

    // Registrar tiempo en archivo (append): "N=<N> P=<P> <segundos>"
    FILE *f = fopen(outfile, "a");
    if (!f) {
        shm_free(A, bytes); shm_free(B, bytes); shm_free(C, bytes);
        return 7;
    }
    fprintf(f, "N=%d P=%d %.6f\n", N, P, elapsed);
    fclose(f);

    shm_free(A, bytes); shm_free(B, bytes); shm_free(C, bytes);
    return 0;
}
