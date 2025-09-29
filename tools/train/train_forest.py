#!/usr/bin/env python3
"""Train RandomForest + calibrator on synthetic data with grouped splits."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from sklearn.calibration import CalibratedClassifierCV
from sklearn.ensemble import RandomForestClassifier
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import classification_report, precision_recall_fscore_support, roc_auc_score
from sklearn.model_selection import GroupKFold, GroupShuffleSplit

THIS_DIR = Path(__file__).resolve().parent
SIM_DIR = (THIS_DIR / "../sim").resolve()
sys.path.insert(0, str(SIM_DIR))

from generator import GeneratorConfig, generate_samples  # noqa: E402


def export_forest(model: RandomForestClassifier, path: Path) -> None:
    trees = model.estimators_
    with path.open("w") as f:
        f.write(f"n_trees {len(trees)}\n")
        for tr in trees:
            t = tr.tree_
            f.write(f"tree {t.node_count}\n")
            for i in range(t.node_count):
                f_idx = int(t.feature[i])
                thr = float(t.threshold[i])
                l = int(t.children_left[i])
                r = int(t.children_right[i])
                leaf = l == -1 and r == -1
                if leaf:
                    counts = t.value[i][0]
                    total = counts.sum() + 1e-9
                    p = counts / total
                else:
                    p = np.array([0.34, 0.33, 0.33])
                f.write(
                    f"{i} {f_idx} {thr:.6f} {l} {r} {p[0]:.6f} {p[1]:.6f} {p[2]:.6f}\n"
                )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seed", type=int, default=0, help="random seed")
    parser.add_argument("--groups", type=int, default=240, help="number of synthetic traffic groups")
    parser.add_argument("--noise", type=float, default=0.05, help="label noise probability")
    parser.add_argument("--test-size", type=float, default=0.25, help="held-out fraction")
    parser.add_argument(
        "--forest-path",
        type=Path,
        default=Path("exported_forest.model"),
        help="path to write the forest model",
    )
    parser.add_argument(
        "--calibrator-path",
        type=Path,
        default=Path("exported_calibrator.cfg"),
        help="path to write calibrator weights",
    )
    return parser.parse_args()


def _rule_score_from_bits(bits: np.ndarray) -> np.ndarray:
    ints = bits.astype(np.uint32)
    scores = np.array([int(b).bit_count() for b in ints], dtype=np.float32)
    return np.clip(scores / 4.0, 0.0, 1.0)


def _novelty_from_probs(probs: np.ndarray, threshold: float = 0.5) -> np.ndarray:
    return (probs.max(axis=1) < threshold).astype(np.float32)


def main() -> None:
    args = parse_args()
    cfg = GeneratorConfig(n_groups=args.groups, label_noise=args.noise, seed=args.seed)
    X, y, groups = generate_samples(cfg)

    splitter = GroupShuffleSplit(n_splits=1, test_size=args.test_size, random_state=args.seed)
    train_idx, test_idx = next(splitter.split(X, y, groups))
    Xtr, Xte = X[train_idx], X[test_idx]
    ytr, yte = y[train_idx], y[test_idx]
    gtr = groups[train_idx]

    rf_params = dict(
        n_estimators=64,
        max_depth=8,
        min_samples_leaf=5,
        class_weight={0: 1.0, 1: 2.0, 2: 2.0},
        random_state=args.seed,
        n_jobs=-1,
    )

    rf = RandomForestClassifier(**rf_params)
    rf.fit(Xtr, ytr)

    # Build grouped splits for calibration and logistic fitting
    unique_groups = np.unique(gtr)
    if unique_groups.size >= 2:
        n_splits = min(5, unique_groups.size)
        group_splits = list(GroupKFold(n_splits=n_splits).split(Xtr, ytr, groups=gtr))
    else:
        # Fallback: use repeated grouped shuffle for degenerate cases
        splitter = GroupShuffleSplit(n_splits=3, test_size=0.25, random_state=args.seed)
        group_splits = list(splitter.split(Xtr, ytr, groups=gtr))

    # Probability calibration for evaluation
    calibrator = CalibratedClassifierCV(
        RandomForestClassifier(**rf_params),
        method="sigmoid",
        cv=group_splits,
    )
    calibrator.fit(Xtr, ytr)

    report = classification_report(yte, calibrator.predict(Xte), digits=3)
    print(report)

    # Out-of-fold probabilities for logistic combiner fitting
    oof_probs = np.zeros((len(Xtr), 3), dtype=np.float32)
    for train_idx_fold, val_idx_fold in group_splits:
        rf_fold = RandomForestClassifier(**rf_params)
        rf_fold.fit(Xtr[train_idx_fold], ytr[train_idx_fold])
        oof_probs[val_idx_fold] = rf_fold.predict_proba(Xtr[val_idx_fold])

    p_cyber_oof = oof_probs[:, 1]
    rule_scores_oof = _rule_score_from_bits(Xtr[:, 17])
    novelty_oof = _novelty_from_probs(oof_probs)
    logistic_features = np.column_stack((p_cyber_oof, rule_scores_oof, novelty_oof))
    logistic_targets = (ytr == 1).astype(int)

    logit = LogisticRegression(
        class_weight={0: 1.0, 1: 3.0},
        max_iter=500,
        solver="lbfgs",
    )
    logit.fit(logistic_features, logistic_targets)

    export_forest(rf, args.forest_path)

    coeffs = logit.coef_[0]
    bias = float(logit.intercept_[0])
    calibrator_config = {
        "w_pcyber": float(coeffs[0]),
        "w_rule": float(coeffs[1]),
        "w_novelty": float(coeffs[2]),
        "bias": bias,
    }
    with args.calibrator_path.open("w") as cfg_out:
        for key, value in calibrator_config.items():
            cfg_out.write(f"{key} {value}\n")

    # Evaluate risk fusion on held-out groups
    test_probs = rf.predict_proba(Xte)
    test_features = np.column_stack(
        (
            test_probs[:, 1],
            _rule_score_from_bits(Xte[:, 17]),
            _novelty_from_probs(test_probs),
        )
    )
    risk_scores = logit.predict_proba(test_features)[:, 1]
    cyber_targets = (yte == 1).astype(int)
    risk_preds = (risk_scores >= 0.5).astype(int)
    risk_precision, risk_recall, risk_f1, _ = precision_recall_fscore_support(
        cyber_targets,
        risk_preds,
        average="binary",
        zero_division=0,
    )
    risk_auc = roc_auc_score(cyber_targets, risk_scores)

    artifacts = {
        "train_groups": int(len(np.unique(gtr))),
        "test_groups": int(len(np.unique(groups[test_idx]))),
        "train_samples": int(len(Xtr)),
        "test_samples": int(len(Xte)),
    }
    print(json.dumps(artifacts, indent=2))

    risk_metrics = {
        "risk_precision": float(risk_precision),
        "risk_recall": float(risk_recall),
        "risk_f1": float(risk_f1),
        "risk_auc": float(risk_auc),
    }
    print(json.dumps(risk_metrics, indent=2))


if __name__ == "__main__":
    main()
