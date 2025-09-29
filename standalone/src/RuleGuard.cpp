#include "RuleGuard.hpp"
#include <sstream>
#include <algorithm>
static inline int popcount32(unsigned int x){ return __builtin_popcount(x); }
void RuleGuard::setBits(unsigned int b){ bits=b; }
double RuleGuard::rulescore() const{
    int h = popcount32(bits);
    return h>0 ? std::min(1.0, h/4.0) : 0.0;
}
std::string RuleGuard::reason() const{
    if(bits==0) return "no-rule-hit";
    std::ostringstream s; s<<"rules:";
    if(bits&1) s<<"param ";
    if(bits&2) s<<"rate ";
    if(bits&4) s<<"replay ";
    if(bits&8) s<<"mode ";
    return s.str();
}
