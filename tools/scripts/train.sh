#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/common.sh"

# Install training deps
uv pip install -q numpy scikit-learn

cd "$REPO_DIR/tools/train"
python3 train_forest.py | tee "$REPO_DIR/train_report.txt"

# Copy outputs into Detector config
cp -f exported_forest.model "$REPO_DIR/deployments/DetectorRB3/config/forest.model"
cp -f exported_calibrator.cfg "$REPO_DIR/deployments/DetectorRB3/config/calibrator.cfg"

echo "Updated model + calibrator in deployments/DetectorRB3/config/"

