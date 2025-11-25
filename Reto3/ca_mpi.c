#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

/**
 * Inicializa la carretera completa (solo en rank 0).
 * road: array de tamaño N
 * rho: densidad de carros (0..1)
 */
void init_road(int *road, int N, double rho) {
    for (int i = 0; i < N; ++i) {
        double r = (double)rand() / (double)RAND_MAX;
        road[i] = (r < rho) ? 1 : 0;
    }
}

/**
 * Cuenta carros en un tramo.
 */
int count_cars(const int *road, int N) {
    int total = 0;
    for (int i = 0; i < N; ++i) {
        total += road[i];
    }
    return total;
}

/**
 * Función que calcula el nuevo estado local usando la regla del triplete (L,C,R).
 * local_road: estado actual en este proceso (celdas 0..local_N-1)
 * new_local : estado siguiente
 * left_ghost, right_ghost: celdas vecinas externas
 * local_moves_out: número de carros que se movieron en este proceso
 */
void step_local(const int *local_road,
                int *new_local,
                int local_N,
                int left_ghost,
                int right_ghost,
                int *local_moves_out)
{
    int moves = 0;

    // Inicializamos nuevo estado a 0
    for (int i = 0; i < local_N; ++i) {
        new_local[i] = 0;
    }

    // Primero, contar movimientos: un carro se mueve si C==1 y R==0
    for (int i = 0; i < local_N; ++i) {
        int C = local_road[i];
        if (C == 1) {
            int R;
            if (i == local_N - 1)
                R = right_ghost;
            else
                R = local_road[i + 1];

            if (R == 0)
                moves++;
        }
    }

    // Ahora aplicar la regla del triplete para construir new_local
    for (int i = 0; i < local_N; ++i) {
        int C = local_road[i];
        int L, R;

        if (i == 0)
            L = left_ghost;
        else
            L = local_road[i - 1];

        if (i == local_N - 1)
            R = right_ghost;
        else
            R = local_road[i + 1];

        // Regla:
        // - Si C==0 y L==1 -> un carro entra desde la izquierda (se mueve)
        // - Si C==1 y R==1 -> el carro se queda
        int new_val = 0;
        if ((C == 0 && L == 1) || (C == 1 && R == 1))
            new_val = 1;

        new_local[i] = new_val;
    }

    *local_moves_out = moves;
}

int main(int argc, char **argv) {
    int rank, size;
    int N = 100000;      // tamaño total carretera
    int T = 1000;        // número de iteraciones
    double rho = 0.3;    // densidad de carros
    unsigned int seed = (unsigned int)time(NULL);

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Lectura de parámetros solo en rank 0
    if (rank == 0) {
        if (argc > 1) N   = atoi(argv[1]);
        if (argc > 2) T   = atoi(argv[2]);
        if (argc > 3) rho = atof(argv[3]);
        if (argc > 4) seed = (unsigned int)atoi(argv[4]);

        if (N <= 0 || T <= 0 || rho < 0.0 || rho > 1.0) {
            fprintf(stderr, "Uso: %s [N] [T] [rho] [seed]\n", argv[0]);
            fprintf(stderr, "  N    = tamaño de la carretera (entero > 0)\n");
            fprintf(stderr, "  T    = iteraciones de tiempo (entero > 0)\n");
            fprintf(stderr, "  rho  = densidad inicial de carros (0.0 .. 1.0)\n");
            fprintf(stderr, "  seed = semilla del RNG (opcional)\n");
            // Si hay parámetros malos, abortamos MPI limpiamente
            if (N <= 0 || T <= 0 || rho < 0.0 || rho > 1.0) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }
        }
    }

    // Broadcast de parámetros a todos los procesos
    MPI_Bcast(&N,   1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&T,   1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&rho, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&seed,1, MPI_UNSIGNED, 0, MPI_COMM_WORLD);

    // Cada proceso puede inicializar su RNG si lo necesita
    srand(seed + rank);

    // Cálculo de la distribución de celdas
    int base = N / size;
    int rem  = N % size;

    int local_N = base + (rank < rem ? 1 : 0);

    if (local_N == 0) {
        // Para este ejercicio asumimos N >= size
        if (rank == 0) {
            fprintf(stderr, "Error: hay más procesos que celdas.\n");
        }
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Solo el rank 0 tiene el arreglo global
    int *global_road = NULL;
    int *sendcounts  = NULL;
    int *displs      = NULL;

    if (rank == 0) {
        global_road = (int*)malloc(N * sizeof(int));
        if (!global_road) {
            fprintf(stderr, "Error: no se pudo reservar memoria global.\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        // Inicializar carretera global
        init_road(global_road, N, rho);

        // Arreglos para Scatterv
        sendcounts = (int*)malloc(size * sizeof(int));
        displs     = (int*)malloc(size * sizeof(int));
        if (!sendcounts || !displs) {
            fprintf(stderr, "Error: no se pudo reservar memoria para sendcounts/displs.\n");
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        int offset = 0;
        for (int r = 0; r < size; ++r) {
            int ln = base + (r < rem ? 1 : 0);
            sendcounts[r] = ln;
            displs[r] = offset;
            offset += ln;
        }
    }

    // Arreglos locales
    int *local_road = (int*)malloc(local_N * sizeof(int));
    int *new_local  = (int*)malloc(local_N * sizeof(int));

    if (!local_road || !new_local) {
        fprintf(stderr, "Rank %d: Error al reservar memoria local.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // Distribuir carretera inicial a todos los procesos
    MPI_Scatterv(global_road, sendcounts, displs, MPI_INT,
                 local_road, local_N, MPI_INT,
                 0, MPI_COMM_WORLD);

    // Ya podemos liberar el global_road si no lo necesitamos más
    if (rank == 0) {
        free(global_road);
        free(sendcounts);
        free(displs);
    }

    // Cálculo de carros totales
    int local_cars = count_cars(local_road, local_N);
    int total_cars = 0;
    MPI_Reduce(&local_cars, &total_cars, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    // difundir total_cars a todos
    MPI_Bcast(&total_cars, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("# MPI CA: N = %d, T = %d, rho = %.3f, carros = %d, procesos = %d\n",
               N, T, rho, total_cars, size);
        printf("# t\tvelocidad\n");
    }

    double t0 = MPI_Wtime();

    int left  = (rank - 1 + size) % size;
    int right = (rank + 1) % size;

    for (int t = 0; t < T; ++t) {
        int left_ghost, right_ghost;

        // Intercambio de halos
        // 1) enviar primera celda al vecino izquierdo, recibir primera del vecino derecho
        MPI_Sendrecv(&local_road[0],         1, MPI_INT, left,  0,
                     &right_ghost,           1, MPI_INT, right, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // 2) enviar última celda al vecino derecho, recibir última del vecino izquierdo
        MPI_Sendrecv(&local_road[local_N-1], 1, MPI_INT, right, 1,
                     &left_ghost,            1, MPI_INT, left,  1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        int local_moves = 0;
        step_local(local_road, new_local, local_N, left_ghost, right_ghost, &local_moves);

        // Reducimos movimientos totales
        int global_moves = 0;
        MPI_Reduce(&local_moves, &global_moves, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        if (rank == 0) {
            double v = (total_cars > 0) ? (double)global_moves / (double)total_cars : 0.0;
            printf("%d\t%.6f\n", t, v);
        }

        // Intercambiamos buffers local_road y new_local
        int *tmp = local_road;
        local_road = new_local;
        new_local  = tmp;
    }

    double t1 = MPI_Wtime();
    double elapsed = t1 - t0;

    if (rank == 0) {
        fprintf(stderr, "Tiempo total de simulacion (MPI): %.6f s\n", elapsed);
    }

    free(local_road);
    free(new_local);

    MPI_Finalize();
    return 0;
}
