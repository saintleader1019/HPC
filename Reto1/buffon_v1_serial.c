#define _POSIX_C_SOURCE 199309L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.h"

int main(int argc, char** argv){
    if (argc < 4) {
        fprintf(stderr, "Uso: %s N T outfile\n", argv[0]);
        return 1;
    }
    long long N = atoll(argv[1]);
    (void)argv[2];
    const char* outfile = argv[3];

    const double L = 1.0, D = 1.0;

    rng64_t rng = { .s = 0x243f6a8885a308d3ULL };
    long long crosses = 0;

    double t0 = sec_now();
    for (long long i = 0; i < N; ++i){
        double theta = M_PI * rand01(&rng);   // θ ~ U[0, π)
        double y     = (D * 0.5) * rand01(&rng); // y ~ U[0, D/2)
        if (0.5 * L * fabs(sin(theta)) >= y) crosses++;
    }
    double t1 = sec_now();
    double elapsed = t1 - t0;

    FILE *f = fopen(outfile, "a");
    if (f) {
        fprintf(f, "N=%lld %.6f\n", N, elapsed);
        fclose(f);
    } else {
        perror("fopen");
        return 2;
    }
    return 0;
}
