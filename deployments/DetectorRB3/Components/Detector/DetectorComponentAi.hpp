#pragma once
#include <vector>
#include <string>
struct FeatureFrame { double ts; std::vector<double> x; unsigned int guard_bits; };
class Forest {
public: bool load(const std::string& path); std::vector<double> proba(const std::vector<double>& x) const;
private: struct Node{int f; double t; int l; int r; bool leaf; double p[3];}; struct Tree{std::vector<Node> n;}; std::vector<Tree> trees;
};
class Calibrator {
public: bool load(const std::string& path); double score(double pcyber, double rules, double nov) const;
private: double w_p=2.0, w_r=1.0, w_n=1.0, b=-1.0; static double sig(double z);
};
class RuleGuard {
public: void load(const std::string& allowlist_path); double rulescore(unsigned int guard_bits) const; std::string reason(unsigned int guard_bits) const;
};
class DetectorComponentAi {
public: DetectorComponentAi(); void ingest(const FeatureFrame& f); double lastRisk() const; std::string lastReason() const;
private: Forest forest; Calibrator calib; RuleGuard rules; double last_risk=0.0; std::string last_reason;
};
