#pragma once
#include <string>
class RuleGuard {
public: void setBits(unsigned int bits); double rulescore() const; std::string reason() const;
private: unsigned int bits=0;
};
