#!/bin/bash
set -euo pipefail

# ============== CONFIGURACIÓN ==============
# Pon aquí los ejecutables (rutas) y sus etiquetas
EXES=( "./dart_v3_fork0" "./dart_v3_fork1" "./buffon_v3_fork0" "./buffon_v3_fork1" )
NAMES=( "dart_fork0" "dart_fork1" "buffon_fork0" "buffon_fork1" )
T=( 2 4 8 )

# Tamaños a probar
NS=( 100000000 500000000 1000000000 5000000000 )

# Repeticiones del conjunto completo de tamaños
REPS=10

# Carpeta/archivos de salida
OUT_DIR="resultados_fork_pi"
mkdir -p "$OUT_DIR"
# ===========================================

run_suite() {
  local exe="$1"
  local name="$2"
  local out_file="$OUT_DIR/tiempos_${name}.txt"

  # limpiar archivo
  : > "$out_file"

  echo ">>> Versión: $name ($exe)"
  for rep in $(seq 1 "$REPS"); do
    echo "  Repetición global $rep..."
    for t in "${T[@]}"; do
      echo " Numero de hilos o procesos $t..."
      for n in "${NS[@]}"; do
        echo "    Ejecutando N=$n..."
        "$exe" "$n" "$t" "$out_file"
      done
    done
  done

  echo ">>> Terminado $name. Salida en: $out_file"
}

# Corre la batería para cada versión
for i in "${!EXES[@]}"; do
  run_suite "${EXES[$i]}" "${NAMES[$i]}"
done
