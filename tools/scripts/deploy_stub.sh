#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# Example deploy: copy DetectorRB3 binary + config to a destination directory
DEST_DIR="${1:-$REPO_DIR/deploy_out/DetectorRB3}"
mkdir -p "$DEST_DIR/config"

WS_DIR="$(cd "$REPO_DIR/.." && pwd)"
FRAME_DIR="$WS_DIR/fprime"

BIN_SRC="$FRAME_DIR/DetectorRB3/build-fprime-automatic-native/bin/Darwin/DetectorRB3"
CONF_SRC="$REPO_DIR/deployments/DetectorRB3/config"

if [ ! -x "$BIN_SRC" ]; then
  echo "DetectorRB3 binary not found. Build first: tools/scripts/build_fprime.sh" >&2
  exit 1
fi

cp -f "$BIN_SRC" "$DEST_DIR/DetectorRB3"
rsync -a "$CONF_SRC/" "$DEST_DIR/config/"
echo "Deployed DetectorRB3 to $DEST_DIR"

