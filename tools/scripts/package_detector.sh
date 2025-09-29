#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

DEST_DIR="${1:-$REPO_DIR/deploy_out/DetectorRB3}"
mkdir -p "$DEST_DIR/config"

# Prefer an F´ DetectorRB3 binary if one was built; otherwise fall back to standalone
BIN_CANDIDATES=(
  "$REPO_DIR/../fprime/DetectorRB3/build-fprime-automatic-*/bin/*/DetectorRB3"
  "$REPO_DIR/../fprime/DetectorRB3/build-fprime-automatic-*/bin/DetectorRB3"
  "$REPO_DIR/DetectorRB3"
  "$REPO_DIR/standalone/detector_main"
)

BIN_SRC=""
for pat in "${BIN_CANDIDATES[@]}"; do
  for f in $(ls -1d $pat 2>/dev/null || true); do
    if [ -x "$f" ]; then BIN_SRC="$f"; break; fi
  done
  [ -n "$BIN_SRC" ] && break
done

if [ -z "$BIN_SRC" ]; then
  echo "No detector binary found. Build either F′ DetectorRB3 or standalone first." >&2
  exit 1
fi

cp -f "$BIN_SRC" "$DEST_DIR/"
rsync -a "$REPO_DIR/deployments/DetectorRB3/config/" "$DEST_DIR/config/"

cat > "$DEST_DIR/run.sh" <<'SH'
#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
BIN="$HERE/DetectorRB3"
if [ -x "$BIN" ]; then
  exec "$BIN"
fi
BIN="$HERE/detector_main"
if [ -x "$BIN" ]; then
  # Standalone expects a CSV path or stdin. Example:
  if [ $# -gt 0 ]; then exec "$BIN" "$@"; else exec "$BIN"; fi
fi
echo "No runnable binary found (DetectorRB3 or detector_main)" >&2
exit 1
SH
chmod +x "$DEST_DIR/run.sh"

echo "Packaged detector to: $DEST_DIR"

