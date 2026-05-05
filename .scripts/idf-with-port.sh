#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 <idf.py args...>" >&2
  exit 2
fi

# Load ESP-IDF toolchain/environment for this shell.
. /sdk/esp-idf/export.sh >/dev/null 2>&1

pick_port() {
  local candidate
  for candidate in /dev/ttyUSB* /dev/ttyACM*; do
    if [ -e "$candidate" ]; then
      echo "$candidate"
      return 0
    fi
  done
  return 1
}

PORT="${ESPPORT:-}"
if [ -z "$PORT" ]; then
  if ! PORT="$(pick_port)"; then
    echo "No serial port found (looked for /dev/ttyUSB* and /dev/ttyACM*)." >&2
    echo "Connect the board or set ESPPORT, for example: ESPPORT=/dev/ttyUSB0" >&2
    exit 2
  fi
fi

exec idf.py -p "$PORT" "$@"
