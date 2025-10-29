#!/bin/bash
set -euo pipefail

# ================= CONFIGURACIÓN =================
# Rutas de los ejecutables (ajusta si los moviste)
EXES=( "./mm_seq" )
NAMES=( "mm_seq" )

# Cantidad de hilos OpenMP a probar
#T=( 2 4 8 )

# Tamaños de matrices
NS=(1024 2048 4096)

# Repeticiones del conjunto completo
REPS=10

# Carpeta de salida
OUT_DIR="resultados_mm_seq"
mkdir -p "$OUT_DIR"
# =================================================

run_suite() {
  local exe="$1"
  local name="$2"
  local out_file="$OUT_DIR/tiempos_${name}.txt"

  # Limpia archivo previo
  : > "$out_file"

  echo ">>> Versión: $name ($exe)"
  for rep in $(seq 1 "$REPS"); do
    echo "  Repetición global $rep..."
    #for t in "${T[@]}"; do
      #echo "  Hilos: $t..."
      for n in "${NS[@]}"; do
        echo "    Ejecutando N=$n..."
        "$exe" "$n" "$out_file"

        if [[ -f gmon.out ]]; then
          new_gmon="${OUT_DIR}/gmon_${name}_N${n}_rep${rep}.out"
          mv gmon.out "$new_gmon"
          echo "      → guardado perfil: $new_gmon"
        fi

      done
    #done
  done

  echo ">>> Terminado $name. Resultados en: $out_file"
}

# Ejecutar las dos versiones
for i in "${!EXES[@]}"; do
  run_suite "${EXES[$i]}" "${NAMES[$i]}"
done