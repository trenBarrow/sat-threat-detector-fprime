#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# Ensure framework checkout next to this repo
WS_DIR="$(cd "$REPO_DIR/.." && pwd)"
FRAME_DIR="$WS_DIR/fprime"
if [ ! -d "$FRAME_DIR/.git" ]; then
  git clone --depth 1 https://github.com/nasa/fprime.git "$FRAME_DIR"
fi

# Sync deployments into framework tree
rsync -a --delete "$REPO_DIR/deployments/RefSat/" "$FRAME_DIR/RefSat/"
rsync -a --delete "$REPO_DIR/deployments/DetectorRB3/" "$FRAME_DIR/DetectorRB3/"

# Build RefSat and DetectorRB3 without Java: use native fpp binaries shipped with fprime-fpp
export FPRIME_SKIP_TOOLS_VERSION_CHECK=1

pushd "$FRAME_DIR/RefSat" >/dev/null
  fprime-util purge -f -r . || true
  fprime-util generate -r . -DFPRIME_FRAMEWORK_PATH=..
  fprime-util build -r . -j "${NINJA_JOBS:-4}"
  echo "RefSat built: $FRAME_DIR/RefSat/build-fprime-automatic-native/bin/Darwin/RefSat"
popd >/dev/null

pushd "$FRAME_DIR/DetectorRB3" >/dev/null
  fprime-util purge -f -r . || true
  fprime-util generate -r . -DFPRIME_FRAMEWORK_PATH=..
  fprime-util build -r . -j "${NINJA_JOBS:-4}"
  echo "DetectorRB3 built: $FRAME_DIR/DetectorRB3/build-fprime-automatic-native/bin/Darwin/DetectorRB3"
popd >/dev/null
