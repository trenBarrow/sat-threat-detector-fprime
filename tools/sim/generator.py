"""Shared synthetic dataset generator for detector training and simulation."""
from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, Tuple

import numpy as np

# Ordered feature names matching deployments/DetectorRB3/config/feature_schema.csv (minus timestamp)
FEATURE_NAMES: Tuple[str, ...] = (
    "bytes_per_s",
    "pkts_per_s",
    "iat_p50_ms",
    "iat_p95_ms",
    "retrans_pct",
    "ttl_var",
    "win_var",
    "flow_delta",
    "fivetuple_changes",
    "opcode",
    "subsystem",
    "mode",
    "param_bucket",
    "seq_gap",
    "resp_delay_ms",
    "ack_flag_rate",
    "rule_score",  # reserved / derived rule confidence slot
    "guard_violation_bits",
)

# Indices that should be treated as integers when exporting to CSV
DISCRETE_INDICES = {8, 9, 10, 11, 12, 17}

# Column ordering used in CSV exports (rule_score is omitted in on-board CSV schema)
CSV_EXPORT_INDICES: Tuple[int, ...] = tuple(list(range(16)) + [17])
CSV_EXPORT_FEATURES: Tuple[str, ...] = tuple(FEATURE_NAMES[i] for i in CSV_EXPORT_INDICES)


@dataclass(frozen=True)
class GeneratorConfig:
    n_groups: int = 240
    samples_per_group: Tuple[int, int] = (8, 24)
    label_probs: Tuple[float, float, float] = (0.68, 0.22, 0.10)
    label_noise: float = 0.05
    seed: int = 0


def _clamp(value: float, lo: float | None = None, hi: float | None = None) -> float:
    if lo is not None and value < lo:
        value = lo
    if hi is not None and value > hi:
        value = hi
    return value


def _draw_base_features(rng: np.random.Generator) -> np.ndarray:
    x = np.zeros(18, dtype=np.float32)
    x[0] = _clamp(rng.normal(2.1e5, 5.8e4), 5e4)
    x[1] = _clamp(rng.normal(155.0, 28.0), 5.0)
    x[2] = _clamp(rng.normal(11.0, 3.0), 1.0)
    x[3] = _clamp(rng.normal(21.0, 6.0), 3.0)
    x[4] = _clamp(rng.normal(0.018, 0.012), 0.0, 0.6)
    x[5] = _clamp(rng.normal(1.1, 0.35), 0.05)
    x[6] = _clamp(rng.normal(2.1, 0.6), 0.05)
    x[7] = rng.normal(0.0, 1.1)
    x[8] = _clamp(rng.normal(0.2, 0.6), 0.0, 8.0)
    x[9] = _clamp(rng.normal(24.0, 10.0), 1.0, 60.0)
    x[10] = _clamp(rng.normal(3.0, 1.2), 1.0, 8.0)
    x[11] = _clamp(rng.normal(2.0, 0.6), 1.0, 5.0)
    x[12] = _clamp(rng.normal(3.0, 2.5), 0.0, 10.0)
    x[13] = rng.normal(0.0, 1.3)
    x[14] = _clamp(rng.normal(70.0, 35.0), 5.0)
    x[15] = _clamp(rng.normal(0.45, 0.22), 0.0, 1.0)
    # rule_score placeholder; populated after guard bits derived
    x[16] = 0.0
    x[17] = 0.0
    return x


def _apply_label_modifications(
    x: np.ndarray,
    label: int,
    rng: np.random.Generator,
) -> None:
    if label == 1:  # cyber-esque
        x[4] = _clamp(x[4] + rng.normal(0.055, 0.03), 0.0, 0.95)
        x[8] = _clamp(x[8] + rng.normal(1.4, 0.9), 0.0, 12.0)
        x[14] = _clamp(x[14] + rng.normal(40.0, 80.0), 10.0, 800.0)
        x[15] = _clamp(x[15] + rng.normal(0.18, 0.15), 0.0, 1.0)
    elif label == 2:  # non-cyber fault
        x[2] = _clamp(x[2] + rng.normal(6.0, 3.0), 1.0, 40.0)
        x[3] = _clamp(x[3] + rng.normal(12.0, 5.0), 5.0, 90.0)
        x[13] += rng.normal(3.2, 1.8)
        x[14] = _clamp(x[14] + rng.normal(420.0, 240.0), 30.0, 1600.0)
    else:  # benign background
        x[7] += rng.normal(0.0, 0.6)
        x[14] = _clamp(x[14] + rng.normal(-10.0, 50.0), 5.0)

    x[0] = _clamp(x[0] + rng.normal(0.0, 2.7e4), 2e4, 6e5)
    x[1] = _clamp(x[1] + rng.normal(0.0, 18.0), 0.0, 400.0)
    x[6] = _clamp(x[6] + rng.normal(0.0, 0.45), 0.02, 6.0)


def _guard_bits_for(label: int, rng: np.random.Generator) -> int:
    if label == 1:
        bits = 0
        if rng.random() < 0.65:
            bits |= 0b010  # rate guard
        if rng.random() < 0.40:
            bits |= 0b001  # param guard
        if rng.random() < 0.18:
            bits |= 0b100  # replay guard
        return bits
    if label == 2:
        bits = 0
        if rng.random() < 0.35:
            bits |= 0b001
        if rng.random() < 0.20:
            bits |= 0b100
        return bits
    # benign: occasional nuisance hits
    return 1 if rng.random() < 0.06 else 0


def generate_samples(config: GeneratorConfig | None = None) -> Tuple[np.ndarray, np.ndarray, np.ndarray]:
    cfg = config or GeneratorConfig()
    rng = np.random.default_rng(cfg.seed)

    features: list[np.ndarray] = []
    labels: list[int] = []
    groups: list[int] = []

    group_labels = rng.choice(3, size=cfg.n_groups, p=cfg.label_probs)

    for group_id, base_label in enumerate(group_labels):
        count = int(rng.integers(cfg.samples_per_group[0], cfg.samples_per_group[1] + 1))
        for _ in range(count):
            x = _draw_base_features(rng)
            _apply_label_modifications(x, base_label, rng)
            label = base_label
            if rng.random() < cfg.label_noise:
                noisy = int(rng.integers(0, 3))
                label = noisy
            guard_bits = _guard_bits_for(label, rng)
            x[17] = float(guard_bits)
            x[16] = float(min(1.0, bin(guard_bits).count("1") / 4.0))

            # Snap discrete fields closer to their integer domains
            x[8] = float(int(round(_clamp(x[8], 0.0, 12.0))))
            x[9] = float(int(round(_clamp(x[9], 1.0, 60.0))))
            x[10] = float(int(round(_clamp(x[10], 1.0, 12.0))))
            x[11] = float(int(round(_clamp(x[11], 1.0, 6.0))))
            x[12] = float(int(round(_clamp(x[12], 0.0, 15.0))))

            features.append(x)
            labels.append(label)
            groups.append(group_id)

    return np.array(features, dtype=np.float32), np.array(labels, dtype=np.int64), np.array(groups, dtype=np.int64)


def iter_csv_rows(X: np.ndarray) -> Iterable[Iterable[float]]:
    for row in X:
        yield [row[idx] for idx in CSV_EXPORT_INDICES]


__all__ = [
    "GeneratorConfig",
    "FEATURE_NAMES",
    "CSV_EXPORT_FEATURES",
    "CSV_EXPORT_INDICES",
    "generate_samples",
    "iter_csv_rows",
]
