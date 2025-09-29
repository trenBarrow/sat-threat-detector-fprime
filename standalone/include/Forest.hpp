#pragma once
#include <vector>
#include <string>
struct ForestNode { int f; double t; int l; int r; bool leaf; double p[3]; };
struct ForestTree { std::vector<ForestNode> n; };
class Forest {
public: bool load(const std::string& path); std::vector<double> proba(const std::vector<double>& x) const;
private: std::vector<ForestTree> trees;
};
