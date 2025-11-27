#!/usr/bin/env bash
set -euo pipefail

# -------------------------------
# Configuración del experimento
# -------------------------------

# Tamaños de la carretera a probar
NS=(100000 1000000 10000000)           # puedes agregar más: (100000 500000 1000000)
# Iteraciones de tiempo
TS=(2000)              # puedes agregar más: (1000 2000)
# Densidades de carros
RHOS=(0.3 0.7)             # por ejemplo: (0.1 0.3 0.5 0.7)
# Repeticiones por configuración (para promediar luego)
REPS=10

# Semilla base para que las ejecuciones sean reproducibles
SEED_BASE=1234

# -------------------------------
# Preparar CSVs
# -------------------------------

SERIAL_CSV="resultados_serial.csv"

echo "Limpiando CSV anteriores..."
rm -f "$SERIAL_CSV"

# Los encabezados los escriben los propios programas si el archivo no existe,
# así que no hace falta escribirlos aquí.

# -------------------------------
# Verificación de ejecutables
# -------------------------------

if [[ ! -x ./ca_serial ]]; then
  echo "Error: ./ca_serial no existe o no es ejecutable. Compila primero:"
  echo "  gcc -O2 -std=c11 -Wall ca_serial.c -o ca_serial"
  exit 1
fi

# -------------------------------
# Bucle principal de experimentos
# -------------------------------

for N in "${NS[@]}"; do
  for T in "${TS[@]}"; do
    for rho in "${RHOS[@]}"; do

      echo "=============================================="
      echo "Configuración: N=$N, T=$T, rho=$rho"
      echo "=============================================="

      for rep in $(seq 1 "$REPS"); do
        seed=$((SEED_BASE + rep))
        echo "[SERIAL] rep=$rep  N=$N  T=$T  rho=$rho  seed=$seed"
        ./ca_serial "$N" "$T" "$rho" "$seed" > /dev/null
      done

    done
  done
done

echo "Experimentos completados."
echo "Mira los archivos: $SERIAL_CSV y $MPI_CSV"
