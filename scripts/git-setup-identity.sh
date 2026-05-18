#!/usr/bin/env bash
# Configura nome e e-mail do Git UMA VEZ (global) — vale para todos os projetos.
# Uso:
#   ./scripts/git-setup-identity.sh
#   GIT_USER_NAME="Seu Nome" GIT_USER_EMAIL="voce@email.com" ./scripts/git-setup-identity.sh
#
# E-mail privado no GitHub (recomendado):
#   GitHub → Settings → Emails → marque "Keep my email addresses private"
#   use o endereço @users.noreply.github.com que aparecer lá.

set -euo pipefail

if ! command -v git >/dev/null 2>&1; then
  echo "Git nao encontrado." >&2
  exit 1
fi

NAME="${GIT_USER_NAME:-}"
EMAIL="${GIT_USER_EMAIL:-}"

if [[ -z "$NAME" ]]; then
  read -r -p "Nome completo (ex.: Joao Silva): " NAME
fi
if [[ -z "$EMAIL" ]]; then
  read -r -p "E-mail (ou noreply do GitHub): " EMAIL
fi

if [[ -z "$NAME" || -z "$EMAIL" ]]; then
  echo "Nome e e-mail sao obrigatorios." >&2
  exit 1
fi

git config --global user.name "$NAME"
git config --global user.email "$EMAIL"

# Evita pedir identidade de novo em repos novos
git config --global init.defaultBranch main 2>/dev/null || true

# Credenciais: cache por 8h (WSL/Linux) — menos prompts em push
if git config --global credential.helper >/dev/null 2>&1; then
  :
else
  git config --global credential.helper 'cache --timeout=28800' 2>/dev/null || true
fi

echo ""
echo "Configuracao global aplicada:"
git config --global --list | grep -E '^user\.|^init\.defaultBranch' || true
echo ""
echo "Teste neste repo:"
cd "$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
echo "  $(git config user.name) <$(git config user.email)>"
echo ""
echo "Proximo commit nao deve pedir e-mail de novo (em qualquer pasta)."
