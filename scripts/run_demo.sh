#!/usr/bin/env bash
# Sobe server (porta 9100) em background, dispara 3 clientes-bot e mostra tail do log.
set -euo pipefail
PORT=9100

# sobe
./bin/server $PORT logs/server-demo.log & SRV_PID=$!
sleep 0.3

# bots
./bin/client 127.0.0.1 $PORT --msg "/nick ana" logs/ana.log
./bin/client 127.0.0.1 $PORT --msg "/nick bob" logs/bob.log
./bin/client 127.0.0.1 $PORT --msg "olá!"      logs/cli3.log

# mensagens
./bin/client 127.0.0.1 $PORT --msg "tudo bem?" logs/ana2.log
./bin/client 127.0.0.1 $PORT --msg "/list"     logs/list.log

# mostra um pouco do log e encerra
sleep 0.2
tail -n 20 logs/server-demo.log || true
kill "$SRV_PID" 2>/dev/null || true
