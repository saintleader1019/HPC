#!/usr/bin/env bash
set -euo pipefail

# ================= CONFIG =================
EXES=( "./Dart_seq" "./Buffon_seq" )
NAMES=( "Dart_seq"  "Buffon_seq"  )

# Muestras a probar (ajusta si 1e9/5e9 te toma demasiado)
NS=( 100000000 1000000000 5000000000 )

# Repeticiones por cada N
REPS=10

OUT_DIR="resultados_seq"
mkdir -p "$OUT_DIR"

MASTER="$OUT_DIR/resultados_master_seq.csv"
echo "programa,N,threads,rep,pi,time_s" > "$MASTER"
# =========================================

run_seq () {
  local exe="$1" name="$2"
  local csv="$OUT_DIR/${name}.csv"
  : > "$csv"

  # Verifica ejecutable
  if [[ ! -x "$exe" ]]; then
    echo "ERROR: no encuentro ejecutable: $exe (revisa nombre/ruta/permiso +x)" >&2
    exit 1
  fi

  echo ">>> SEQ: $name ($exe)"
  for rep in $(seq 1 "$REPS"); do
    echo "  Repetición $rep..."
    for n in "${NS[@]}"; do
      echo "    N=$n"

      # gprof: un gmon único por corrida (solo para _seq)
      export GMON_OUT_PREFIX="${OUT_DIR}/gmon_${name}_N${n}_rep${rep}"

      # Los binarios SEC **sin prints** appendean: N,threads(=1),pi,time_s
      tmp="$OUT_DIR/tmp_${name}_N${n}_rep${rep}.csv"
      "$exe" "$n" "$tmp"

      # N,threads,pi,time_s  ->  programa,N,threads,rep,pi,time_s
      awk -F',' -v P="$name" -v R="$rep" '{print P","$1","$2","R","$3","$4}' \
        < "$tmp" | tee -a "$csv" >> "$MASTER"
    done
  done
  echo ">>> CSV: $csv"
}

for i in "${!EXES[@]}"; do
  run_seq "${EXES[$i]}" "${NAMES[$i]}"
done

echo "OK -> Master: $MASTER"
