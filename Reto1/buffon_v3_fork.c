#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <string.h>
#include <math.h>
#include "common.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(int argc, char** argv){
    if (argc < 4){
        fprintf(stderr, "Uso: %s N P outfile\n", argv[0]);
        return 1;
    }
    long long N = atoll(argv[1]);
    int P = atoi(argv[2]);
    const char* outfile = argv[3];
    if (P <= 0) P = 1;

    // Parámetros clásicos
    const double L = 1.0, D = 1.0;

    int (*pipes)[2] = malloc((size_t)P * sizeof *pipes);
    if (!pipes){ perror("malloc"); return 2; }

    volatile int* start = mmap(NULL, sizeof(int),
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANON, -1, 0);
    if (start == MAP_FAILED){ perror("mmap"); free(pipes); return 2; }
    *start = 0;

    long long chunk = N / P, rem = N % P;

    for (int i=0;i<P;i++){
        if (pipe(pipes[i]) != 0){ perror("pipe"); return 2; }
        pid_t pid = fork();
        if (pid < 0){ perror("fork"); return 2; }
        if (pid == 0){
            // --- Hijo ---
            close(pipes[i][0]);
            while (!*start) { /* spin */ }
            __sync_synchronize();

            long long a = i*chunk + (i<rem? i : rem);
            long long b = a + chunk + (i<rem?1:0);

            rng64_t rng = { .s = 0x243f6a8885a308d3ULL ^ (uint64_t)(i+1) };

            long long cross = 0;
            for (long long k = a; k < b; ++k){
                (void)k;
                double theta = M_PI * rand01(&rng);
                double y     = (D * 0.5) * rand01(&rng);
                if (0.5 * L * fabs(sin(theta)) >= y) cross++;
            }
            if (write(pipes[i][1], &cross, sizeof(cross)) != sizeof(cross)) { /* ignore */ }
            close(pipes[i][1]);
            _exit(0);
        } else {
            // Padre
            close(pipes[i][1]);
        }
    }

    // levantar barrera y arrancar cronómetro (cómputo puro)
    __sync_synchronize();
    *(int*)start = 1;
    __sync_synchronize();

    double t0 = sec_now();

    long long total = 0, tmp = 0; int status = 0;
    for (int i=0;i<P;i++){
        if (read(pipes[i][0], &tmp, sizeof(tmp)) == sizeof(tmp)) total += tmp;
        close(pipes[i][0]);
        wait(&status);
    }

    double t1 = sec_now();
    double elapsed = t1 - t0;

    FILE *f = fopen(outfile, "a");
    if (f) { fprintf(f, "N=%lld %.6f\n", N, elapsed); fclose(f); }
    else { perror("fopen"); }

    munmap((void*)start, sizeof(int));
    free(pipes);
    return 0;
}
