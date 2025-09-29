#!/usr/bin/env bash
set -euo pipefail

# Resolve repo root
REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/../.. && pwd)"
cd "$REPO_DIR"

# Ensure venv with uv + fprime tools exists
if [ ! -d "$REPO_DIR/.venv" ]; then
  python3 -m venv "$REPO_DIR/.venv"
fi
source "$REPO_DIR/.venv/bin/activate"

# Ensure uv is present for faster installs
if ! command -v uv >/dev/null 2>&1; then
  pip -q install uv
fi

# Minimal build tooling
uv pip install -q --upgrade pip
uv pip install -q fprime-tools ninja cmake

# Optionally install fprime-bootstrap for convenience
uv pip install -q fprime-bootstrap >/dev/null 2>&1 || true

