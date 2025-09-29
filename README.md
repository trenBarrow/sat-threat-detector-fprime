Two‑layer anomaly detector for F´ command/telemetry streams that runs on Snapdragon CPU with and on a laptop in simulation, with Layer‑1 protocol guards plus a compact Random Forest classifier fused by a logistic calibrator to emit a single 0–1 risk and a reason string; it includes a tiny F´ satellite and Detector deployment skeleton so you can build with fprime‑util on Raspberry Pi or run the standalone detector on CSV feature frames without hardware. To use in simulation, run `python3 tools/sim/sim_gen.py > frames.csv` then `make -C standalone` and `./standalone/detector_main frames.csv` which prints one line per frame with timestamp, risk, class, and reason; to deploy, place this `deployments` tree inside an fprime checkout (or add fprime as a sibling), run `fprime-util generate` then `fprime-util build` in `deployments/RefSat` for the reference satellite and in `deployments/DetectorRB3` for the detector, copy the resulting binaries to the Pi and RB3 respectively, and run them with your switch port-mirror capturing in place; replace `config/allowlist_opcodes.txt`, `config/forest.model`, and `config/calibrator.cfg` to tune rules and models, and rebuild only the detector.


Quick layout: `deployments/RefSat` is a bare F´ app that emits periodic telemetry and accepts a PING command; `deployments/DetectorRB3` defines a Detector component stub that would receive fused feature frames and publish a risk channel and alert events for the GDS; `standalone` is a small C++ console that actually scores frames now using a hand‑parsable forest file and a logistic combiner; `tools/sim` generates synthetic CSV frames with benign, cyber, and non‑cyber anomalies; `tools/train` shows how to train a RandomForest on your real fused features and export a `forest.model` in the simple line format this runtime loads. Everything is offline, no system services, and portable; for the lab wiring, mirror sat and GDS ports to the RB3, record pcaps in a ring, and tail the GDS logs to build features as described in the project brief.


F´ build notes: this is a skeleton meant to drop into an fprime workspace; create a workspace `fprime/` next to this repo, initialize per the tutorials, then symlink or copy `deployments/RefSat` and `deployments/DetectorRB3` into `fprime/` and run `fprime-util generate && fprime-util build`; the Detector component is defined in FPP (preferred) with `RiskScore` telemetry channel id `0x7000` and `RiskAlert` event, and includes a true `FeatureIn` handler that ingests feature frames. If you prefer generating FPP from the reference XML at configure-time, turn on the CMake option `-DDETECTOR_USE_XML=ON` (requires `fpp-from-xml` in PATH). The standalone detector provides the exact scoring logic you should call from that component.


Training: use `tools/train/train_forest.py` on your labeled windows to fit a 3‑class RandomForest with class weights and Platt calibration, then write `forest.model` via `export_forest()`; copy the resulting file to `deployments/DetectorRB3/config/forest.model` and keep `calibrator.cfg` synchronized; no live training, copy models by USB only.


Simulation mode: no boards needed; generate CSV frames, build once with `make`, and run; the format is in `deployments/DetectorRB3/config/feature_schema.csv`; detector_main accepts either a file path or stdin.


Safety: this is offline, read‑only, and write‑prints only; rules are strict allowlists, rates, and pairing guards; the forest and calibrator fuse with a sigmoid to produce a stable, single risk with a terse reason string; thresholds are in the config and easy to adjust.


Quick start scripts
- Train: `bash tools/scripts/train.sh` updates `deployments/DetectorRB3/config/{forest.model,calibrator.cfg}` and writes a brief report to `train_report.txt`.
- Simulate: `bash tools/scripts/run_sim.sh` builds the standalone detector, generates `frames.csv`, and prints one result per row.
- Build F´ deployments locally: `bash tools/scripts/build_fprime.sh` (clones nasa/fprime next to the repo if needed, then builds `RefSat` and `DetectorRB3`).
- Package for SoC/USB: `bash tools/scripts/package_detector.sh /path/to/usb/DetectorRB3` (copies a runnable `DetectorRB3` or `detector_main` plus `config/` and a `run.sh`). On device, run `./run.sh <frames.csv>` or pipe your feature stream.
