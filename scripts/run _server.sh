#!/usr/bin/env bash
set -euo pipefail
PORT="${1:-9000}"
LOG="${2:-logs/server.log}"
./bin/server "$PORT" "$LOG"
