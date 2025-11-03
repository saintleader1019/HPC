#!/usr/bin/env bash
set -euo pipefail

EXES=( "./Dart_omp" "./Dart_omp3" "./Buffon_omp" "./Buffon_omp3" )
  NAMES=( "Dart_omp"  "Dart_omp3"  "Buffon_omp"  "Buffon_omp3" )

THREADS=( 2 4 8 )
NS=( 100000000 1000000000 5000000000 )
REPS=10

OUT_DIR="resultados_omp"
mkdir -p "$OUT_DIR"

MASTER="$OUT_DIR/resultados_master.csv"
echo "programa,N,threads,rep,pi,time_s" > "$MASTER"

run_suite () {
  local exe="$1" name="$2"
  local csv="$OUT_DIR/${name}.csv"
  : > "$csv"

  echo ">>> $name ($exe)"
  for rep in $(seq 1 "$REPS"); do
    for t in "${THREADS[@]}"; do
      export GMON_OUT_PREFIX="${OUT_DIR}/gmon_${name}_T${t}_rep${rep}"
      for n in "${NS[@]}"; do
        # cada ejecutable APPENDea: N,threads,pi,time_s
        out_file="$OUT_DIR/tmp_${name}_T${t}_rep${rep}.csv"
        OMP_NUM_THREADS="$t" "$exe" "$n" "$out_file"
        awk -v P="$name" -v R="$rep" -v T="$t" -v N="$n" \
            -F',' '{print P "," $1 "," $2 "," R "," $3 "," $4}' \
            < "$out_file" >> "$csv"
        cat "$csv" | tail -n 1 >> "$MASTER"
      done
    done
  done
  echo ">>> CSV: $csv"
}

for i in "${!EXES[@]}"; do
  run_suite "${EXES[$i]}" "${NAMES[$i]}"
done

echo "OK -> $MASTER"
