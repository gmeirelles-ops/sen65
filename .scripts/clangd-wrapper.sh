#!/usr/bin/env bash
# Faz o clangd carregar /app/.config/clangd/config.yaml (PathMatch para /sdk/esp-idf).
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export XDG_CONFIG_HOME="${ROOT}/.config"
exec clangd "$@"
