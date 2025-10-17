#!/bin/bash
# NO usamos `set -e` para que no se aborte a mitad si un gprof falla
set -u  # sí queremos error por variables no definidas

OUT_DIR="resultados_mm_omp"

# Deben ser los mismos que usaste en run_mm.sh
EXES=( "./mm_omp" "./mm_omp_O3" "./mm_omp_opt_mem" "./mm_omp_opt_mem_O3" )
NAMES=( "mm_omp" "mm_omp_O3" "mm_omp_opt_mem" "mm_omp_opt_mem_O3" )

NS=(1024 2048 4096)
T=(2 4 8)
REPS=10

mkdir -p "$OUT_DIR"

generated=0
missing=0
failed=0

for i in "${!EXES[@]}"; do
  exe="${EXES[$i]}"
  name="${NAMES[$i]}"

  echo "Procesando reportes de ${name} ..."
  if [[ ! -x "$exe" ]]; then
    echo "  ⚠️  Ejecutable no encontrado o no ejecutable: $exe (se omite este nombre)"
    continue
  fi

  for n in "${NS[@]}"; do
    for t in "${T[@]}"; do
      for rep in $(seq 1 "$REPS"); do
        gmon_file="${OUT_DIR}/gmon_${name}_N${n}_T${t}_rep${rep}.out"
        report_file="${OUT_DIR}/perfil_${name}_N${n}_T${t}_rep${rep}.txt"

        if [[ -f "$gmon_file" ]]; then
          echo "  Generando ${report_file} ..."
          # No abortar si gprof falla: continuar con el siguiente
          if gprof "$exe" "$gmon_file" > "$report_file" 2> "${report_file%.txt}.log"; then
            ((generated++))
          else
            echo "    ❌ gprof falló para $gmon_file (ver ${report_file%.txt}.log)"
            ((failed++))
          fi
        else
          echo "  (faltante) ${gmon_file}"
          ((missing++))
        fi
      done
    done
  done
done

echo "==========================================="
echo "✅ Reportes generados: $generated"
echo "❌ gmon.out faltantes: $missing"
echo "⚠️ gprof con error:   $failed"
echo "Archivos de log en:   ${OUT_DIR}/*.log"