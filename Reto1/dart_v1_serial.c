#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

int main(int argc, char** argv){
    long long N = 10000000;     // por defecto: 1e7
    uint64_t seed = 12345;      // semilla por defecto

    for (int i=1; i<argc; ++i){
        if (!strcmp(argv[i], "-n") && i+1<argc) N = atoll(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i+1<argc) seed = strtoull(argv[++i], NULL, 10);
    }

    rng64_t rng = {.s = seed ? seed : 88172645463393265ULL};
    long long hit = 0;

    double t0 = sec_now();
    for (long long i=0; i<N; ++i){
        double x = rand01(&rng);
        double y = rand01(&rng);
        if (x*x + y*y <= 1.0) hit++;
    }
    double t1 = sec_now();

    double pi = 4.0 * (double)hit / (double)N;
    
    return 0;
}