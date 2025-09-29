#pragma once
#include <string>
class Calibrator {
public: bool load(const std::string& path); double score(double pcyber, double rules, double novelty) const;
private: double w_p=2.5, w_r=1.5, w_n=1.0, b=-1.0; static double sig(double z);
};
