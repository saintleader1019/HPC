#!/bin/bash
set -euo pipefail

OUT="tiempos.txt"
: > "$OUT"  # vaciar archivo

sizes=(500 1000 2000 5000)

for iter in {1..10}; do
  for n in "${sizes[@]}"; do
    # sin semilla (usa time(NULL)):
    ./mm_seq_time_log "$n" "$OUT"
    # o con semilla estable por iteración si quieres reproducibilidad:
    # seed=$((1000 * iter + n))
    # ./mm_seq_time_log "$n" "$seed" "$OUT"
  done
done
