#!/usr/bin/env python3
"""Generate synthetic feature frames with overlapping distributions and label noise."""
from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))

from generator import (  # noqa: E402
    CSV_EXPORT_FEATURES,
    GeneratorConfig,
    generate_samples,
    iter_csv_rows,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-g", "--groups", type=int, default=240, help="number of traffic groups to simulate")
    parser.add_argument("-n", "--noise", type=float, default=0.05, help="label noise probability per sample")
    parser.add_argument("-s", "--seed", type=int, default=0, help="random seed")
    parser.add_argument("--start-ts", type=float, default=None, help="starting timestamp (defaults to now)")
    parser.add_argument(
        "--with-labels",
        action="store_true",
        help="append ground-truth label column for offline evaluation",
    )
    parser.add_argument(
        "--with-groups",
        action="store_true",
        help="append group identifier column",
    )
    return parser.parse_args()


def format_field(value: float) -> str:
    if abs(value - round(value)) < 1e-6:
        return str(int(round(value)))
    return f"{value:.6f}"


def main() -> None:
    args = parse_args()

    cfg = GeneratorConfig(n_groups=args.groups, label_noise=args.noise, seed=args.seed)
    X, y, groups = generate_samples(cfg)

    ts0 = args.start_ts if args.start_ts is not None else time.time()
    header = ["ts", *CSV_EXPORT_FEATURES]
    if args.with_labels:
        header.append("label")
    if args.with_groups:
        header.append("group")
    print(",".join(header))

    for idx, feature_row in enumerate(iter_csv_rows(X)):
        ts = ts0 + idx
        row = [f"{ts:.3f}"] + [format_field(val) for val in feature_row]
        if args.with_labels:
            row.append(str(int(y[idx])))
        if args.with_groups:
            row.append(str(int(groups[idx])))
        print(",".join(row))


if __name__ == "__main__":
    main()
