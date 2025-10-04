#!/usr/bin/env bash
# Dispara N clientes que mandam 1 msg cada.
set -euo pipefail
HOST="${1:-127.0.0.1}"
PORT="${2:-9000}"
N="${3:-5}"

for i in $(seq 1 "$N"); do
  ./bin/client "$HOST" "$PORT" --msg "hello from client $i" "logs/client-$i.log" &
done
wait
echo "Enviadas $N mensagens."
