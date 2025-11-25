#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
 * Inicializa la carretera con carros (1) y huecos (0)
 * según una densidad rho en [0, 1].
 */
void init_road(int *road, int N, double rho) {
    int i;
    for (i = 0; i < N; ++i) {
        double r = (double)rand() / (double)RAND_MAX;
        road[i] = (r < rho) ? 1 : 0;
    }
}

/**
 * Cuenta el número total de carros en la carretera.
 */
int count_cars(const int *road, int N) {
    int i, total = 0;
    for (i = 0; i < N; ++i) {
        total += road[i];
    }
    return total;
}

/**
 * Realiza un paso de tiempo del autómata.
 * road      = estado actual
 * new_road  = estado siguiente (se escribe aquí)
 * N         = tamaño de la carretera
 * moves_out = número de carros que se movieron en este paso
 */
void step(const int *road, int *new_road, int N, int *moves_out) {
    int i;
    int moves = 0;

    // Inicializamos el nuevo estado a 0 (todas vacías)
    for (i = 0; i < N; ++i) {
        new_road[i] = 0;
    }

    // Recorremos celdas de la carretera
    for (i = 0; i < N; ++i) {
        if (road[i] == 1) {
            int j = (i + 1 == N) ? 0 : (i + 1);  // celda de adelante (con frontera periódica)
            if (road[j] == 0) {
                // La celda de adelante está vacía: el carro avanza
                new_road[j] = 1;
                moves++;
            } else {
                // La celda de adelante está ocupada: el carro se queda
                new_road[i] = 1;
            }
        }
    }

    *moves_out = moves;
}

/**
 * Medición sencilla de tiempo con clock_gettime (monotónico).
 */
double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(int argc, char **argv) {
    int N = 100000;      // número de celdas de la carretera
    int T = 1000;        // número de iteraciones
    double rho = 0.3;    // densidad de carros (0..1)
    unsigned int seed = (unsigned int)time(NULL);

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
        return EXIT_FAILURE;
    }

    srand(seed);

    int *road     = (int*)malloc(N * sizeof(int));
    int *new_road = (int*)malloc(N * sizeof(int));

    if (!road || !new_road) {
        fprintf(stderr, "Error: no se pudo reservar memoria.\n");
        free(road);
        free(new_road);
        return EXIT_FAILURE;
    }

    // Inicializamos la carretera
    init_road(road, N, rho);

    // Contamos carros una vez (se conserva en el tiempo)
    int total_cars = count_cars(road, N);
    if (total_cars == 0) {
        fprintf(stderr, "Advertencia: no hay carros en la carretera (rho muy baja?).\n");
    }

    printf("# N = %d, T = %d, rho = %.3f, carros = %d\n", N, T, rho, total_cars);
    printf("# t\tvelocidad\n");

    double t0 = get_time_sec();

    // Bucle temporal principal
    for (int t = 0; t < T; ++t) {
        int moves;
        step(road, new_road, N, &moves);

        // Intercambiamos punteros (doble buffer)
        int *tmp = road;
        road = new_road;
        new_road = tmp;

        double v = (total_cars > 0) ? (double)moves / (double)total_cars : 0.0;

        // Imprimir velocidad por iteración (para luego graficar si quieres)
        printf("%d\t%.6f\n", t, v);
    }

    double t1 = get_time_sec();
    double elapsed = t1 - t0;
    fprintf(stderr, "Tiempo total de simulacion: %.6f s\n", elapsed);

    free(road);
    free(new_road);

    return EXIT_SUCCESS;
}