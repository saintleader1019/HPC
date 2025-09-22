#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "common.h"

int main(int argc, char** argv){
    long long N = 10000000;     // por defecto: 1e7
    uint64_t seed = 12345;      // semilla por defecto
    double L = 1.0, D = 1.0;    // caso clásico L <= D

    for (int i=1; i<argc; ++i){
        if (!strcmp(argv[i], "-n") && i+1<argc) N = atoll(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i+1<argc) seed = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "-L") && i+1<argc) L = atof(argv[++i]);
        else if (!strcmp(argv[i], "-D") && i+1<argc) D = atof(argv[++i]);
    }

    if (L > D) {
        // Para la fórmula clásica se asume L <= D; ajustamos para evitar sesgo
        L = D;
    }

    rng64_t rng = {.s = seed ? seed : 1315423911ULL};
    long long crosses = 0;

    double t0 = sec_now();
    for (long long i=0; i<N; ++i){
        double theta = M_PI * rand01(&rng);      // θ ~ U[0, π)
        double y = (D * 0.5) * rand01(&rng);     // y ~ U[0, D/2)
        if (0.5 * L * fabs(sin(theta)) >= y) crosses++;
    }
    double t1 = sec_now();

    double P = (double)crosses / (double)N;
    double pi_est = (P > 0.0) ? (2.0 * L) / (D * P) : NAN;
    
    return 0;
}