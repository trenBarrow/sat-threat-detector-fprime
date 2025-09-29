#include "DetectorComponentAi.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
static inline int popcount32(unsigned int x){ return __builtin_popcount(x); }
bool Forest::load(const std::string& path){ std::ifstream in(path); if(!in) return false; trees.clear(); std::string tag; int T=0; in>>tag>>T; for(int t=0;t<T;++t){ std::string tt; int N; in>>tt>>N; Tree tr; tr.n.resize(N); for(int i=0;i<N;++i){ int idx, f, l, r; double th, p0, p1, p2; in>>idx>>f>>th>>l>>r>>p0>>p1>>p2; tr.n[i].f=f; tr.n[i].t=th; tr.n[i].l=l; tr.n[i].r=r; tr.n[i].leaf = (l<0 && r<0); tr.n[i].p[0]=p0; tr.n[i].p[1]=p1; tr.n[i].p[2]=p2; } trees.push_back(tr);} return true; }
std::vector<double> Forest::proba(const std::vector<double>& x) const{ double a0=0,a1=0,a2=0; for(auto& t:trees){ int i=0; while(!t.n[i].leaf){ const auto& nd=t.n[i]; i = (x[nd.f] <= nd.t) ? nd.l : nd.r; } a0+=t.n[i].p[0]; a1+=t.n[i].p[1]; a2+=t.n[i].p[2]; } double Z = a0+a1+a2; if(Z<=0) return {0.34,0.33,0.33}; return {a0/Z,a1/Z,a2/Z}; }
bool Calibrator::load(const std::string& path){ std::ifstream in(path); if(!in) return false; std::string k; double v; while(in>>k>>v){ if(k=="w_pcyber") w_p=v; else if(k=="w_rule") w_r=v; else if(k=="w_novelty") w_n=v; else if(k=="bias") b=v; } return true; }
double Calibrator::sig(double z){ return 1.0/(1.0+std::exp(-z)); }
double Calibrator::score(double pcyber, double rules, double nov) const{ return sig(w_p*pcyber + w_r*rules + w_n*nov + b); }
void RuleGuard::load(const std::string&){}
double RuleGuard::rulescore(unsigned int guard_bits) const{ int h = popcount32(guard_bits); return h>0 ? std::min(1.0, h/4.0) : 0.0; }
std::string RuleGuard::reason(unsigned int guard_bits) const{ if(guard_bits==0) return "no-rule-hit"; std::ostringstream s; s<<"rules:"; if(guard_bits&1) s<<"param "; if(guard_bits&2) s<<"rate "; if(guard_bits&4) s<<"replay "; if(guard_bits&8) s<<"mode "; return s.str(); }
DetectorComponentAi::DetectorComponentAi(){ forest.load("config/forest.model"); calib.load("config/calibrator.cfg"); }
void DetectorComponentAi::ingest(const FeatureFrame& f){ auto p = forest.proba(f.x); double pcyber = p[1]; double nov = (std::max({p[0],p[1],p[2]})<0.5) ? 1.0 : 0.0; double rs = rules.rulescore(f.guard_bits); last_risk = calib.score(pcyber, rs, nov); std::ostringstream s; int best = (p[1]>p[0] && p[1]>p[2])?1:((p[2]>p[0] && p[2]>p[1])?2:0); s<<"pcyber="<<pcyber<<" class="<<best<<" "<<rules.reason(f.guard_bits)<<" nov="<<(nov>0.5?"y":"n"); last_reason = s.str(); }
double DetectorComponentAi::lastRisk() const{ return last_risk; }
std::string DetectorComponentAi::lastReason() const{ return last_reason; }
