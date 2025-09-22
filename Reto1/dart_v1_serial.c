// dart_serial_mm.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

int main(int argc, char** argv){
    if (argc < 4) {
        fprintf(stderr, "Uso: %s N T outfile\n", argv[0]);
        return 1;
    }
    long long N = atoll(argv[1]);
    (void)argv[2]; // T se ignora en versión secuencial
    const char* outfile = argv[3];

    // Semilla fija para reproducibilidad (puedes cambiarla si quieres)
    rng64_t rng = { .s = 0x9e3779b97f4a7c15ULL };
    long long hit = 0;

    double t0 = sec_now();
    for (long long i = 0; i < N; ++i){
        double x = rand01(&rng);
        double y = rand01(&rng);
        if (x*x + y*y <= 1.0) hit++;
    }
    double t1 = sec_now();
    double elapsed = t1 - t0;

    FILE *f = fopen(outfile, "a");
    if (f) {
        // Formato requerido: "N=<valor> <tiempo>"
        fprintf(f, "N=%lld %.6f\n", N, elapsed);
        fclose(f);
    } else {
        perror("fopen");
        return 2;
    }
    return 0;
}
