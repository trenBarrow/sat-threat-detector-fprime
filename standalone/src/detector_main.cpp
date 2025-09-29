#include "Forest.hpp"
#include "Calibrator.hpp"
#include "RuleGuard.hpp"
#include "FeatureFrame.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
static void split(const std::string& s, char d, std::vector<std::string>& out){ out.clear(); std::stringstream ss(s); std::string tok; while(std::getline(ss,tok,d)) out.push_back(tok); }
static bool exists(const std::string& p){ std::ifstream f(p); return f.good(); }
int main(int argc, char** argv){
    std::string model_path = "deployments/DetectorRB3/config/forest.model";
    std::string calib_path = "deployments/DetectorRB3/config/calibrator.cfg";
    if(!exists(model_path)) model_path = "../deployments/DetectorRB3/config/forest.model";
    if(!exists(calib_path)) calib_path = "../deployments/DetectorRB3/config/calibrator.cfg";
    Forest forest; Calibrator calib; forest.load(model_path); calib.load(calib_path);
    std::istream* in = &std::cin; std::ifstream f;
    if(argc>1){ f.open(argv[1]); if(f) in=&f; }
    std::string line; std::vector<std::string> tok;
    // skip header if present
    if(std::getline(*in,line)){ if(line.find("ts,")!=std::string::npos) { /* header skip */ } else { in->seekg(0); } }
    while(std::getline(*in,line)){
        if(line.empty()) continue;
        split(line, ',', tok);
        if(tok.size()<18) continue; // ts + 17 fields
        FeatureFrame fr; fr.ts = std::stod(tok[0]);
        fr.x.assign(18, 0.0);
        // Map CSV (excluding ts) to model features:
        // 0..15 <- columns 1..16, 16 <- reserved 0.0, 17 <- guard_bits
        for(int i=0;i<16;++i){ fr.x[i] = std::stod(tok[1+i]); }
        fr.guard_bits = static_cast<unsigned int>(std::stoul(tok[17]));
        fr.x[17] = static_cast<double>(fr.guard_bits);
        auto p = forest.proba(fr.x);
        double pcyber = p[1];
        double novelty = (std::max({p[0],p[1],p[2]})<0.5)?1.0:0.0;
        RuleGuard rg; rg.setBits(fr.guard_bits);
        double rs = rg.rulescore();
        double risk = calib.score(pcyber, rs, novelty);
        int cls = (p[1]>p[0] && p[1]>p[2])?1:((p[2]>p[0] && p[2]>p[1])?2:0);
        std::cout<<fr.ts<<","<<risk<<","<<cls<<","<<rg.reason()<<",pcy="<<pcyber<<",nov="<<(novelty>0.5?"y":"n")<<std::endl;
    }
    return 0;
}
