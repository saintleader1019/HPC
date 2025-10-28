gcc -o3 -std=c11 -march=native -pg mm_seq_basic.c -o mm_seq
gcc -O0 -std=c11 -march=native -fopenmp -pg mm_omp.c -o mm_omp
gcc -O3 -std=c11 -march=native -fopenmp -pg mm_omp.c -o mm_omp_O3
gcc -O0 -std=c11 -march=native -fopenmp -pg mm_omp_opt_mem.c -o mm_omp_opt_mem
gcc -O3 -std=c11 -march=native -fopenmp -pg mm_omp_opt_mem.c -o mm_omp_opt_mem_O3