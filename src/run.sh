#!/bin/bash

# ===================================================================
# Script para ejecutar el conjunto de experimentos de la simulación ns-3
# ===================================================================

# --- VALORES POR DEFECTO ---
N=10
MAX_USERS_DEFAULT=500
MAX_BITRATE_DEFAULT=120
MIN_USERS=100
MIN_BITRATE=10
STEP_USERS=50
STEP_BITRATE=10
MAX_USERS=$MAX_USERS_DEFAULT
MAX_BITRATE=$MAX_BITRATE_DEFAULT

# --- PROCESAMIENTO DE OPCIONES ---
NS3_RUN_PREFIX=""
ENABLE_LOGS_ARG="" 

declare -a ARGS_FOR_GETOPTS
for arg in "$@"; do
    if [[ "$arg" == "--log" ]]; then
        NS3_RUN_PREFIX='NS_LOG="OnOffRoutingExperiment=all:Rip=info"'
        ENABLE_LOGS_ARG="--enableLogs=true"
        echo "Opción --log detectada. Se activarán los logs detallados de la simulación."
    else
        ARGS_FOR_GETOPTS+=("$arg")
    fi
done
set -- "${ARGS_FOR_GETOPTS[@]}"

while getopts "u:b:n:" opt; do
  case $opt in
    u) MAX_USERS="$OPTARG"; echo "Tope de usuarios personalizado: $MAX_USERS"; ;;
    b) MAX_BITRATE="$OPTARG"; echo "Tope de bitrate personalizado: $MAX_BITRATE Mbps"; ;;
    n) N="$OPTARG"; echo "Se ejecutarán $N simulaciones por muestra"; ;;
    \?) echo "Opción inválida: -$OPTARG" >&2; exit 1; ;;
    :) echo "La opción -$OPTARG requiere un argumento." >&2; exit 1; ;;
  esac
done

# --- PREPARACIÓN DEL ENTORNO ---
RESULTS_FILE="results.dat"
GRAFICAS_DIR="graficas"
GRAFICAS_PRECISION_DIR="graficas-precision"
echo "Limpiando entorno anterior..."
rm -f $RESULTS_FILE $RESULTS_FILE_PRECISION
rm -rf $GRAFICAS_DIR $GRAFICAS_PRECISION_DIR
touch $RESULTS_FILE
echo "Compilando los programas de simulación y ploteo..."
./ns3 build

# --- BUCLE PRINCIPAL DE SIMULACIÓN ---
for users in $(seq $MIN_USERS $STEP_USERS $MAX_USERS)
do
  echo "----------------------------------------------------"
  echo "Ejecutando simulaciones para $users usuarios..."
  echo "----------------------------------------------------"
  for bitrate_val in $(seq $MIN_BITRATE $STEP_BITRATE $MAX_BITRATE)
  do
    bitrate_str="${bitrate_val}Mbps"
    echo "  -> Bitrate: $bitrate_str"
    for i in $(seq 1 $N)
    do
      semilla=$(($users + $bitrate_val + $i)) 
      echo "    -> Ejecutando réplica $i/$N..."
      
      
      eval $NS3_RUN_PREFIX ./ns3 run "scratch/onoffRouting" -- --num_usuarios=$users --bitrate=$bitrate_str --semilla=$semilla $ENABLE_LOGS_ARG
    done
  done
done

echo ""
echo "Todas las simulaciones han completado."
echo "Generando gráficas finales..."
./ns3 run scratch/plot-results
echo ""
echo "Proceso finalizado. Revisa la carpeta '$GRAFICAS_DIR'."
