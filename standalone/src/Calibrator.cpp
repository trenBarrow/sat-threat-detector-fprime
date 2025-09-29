#include "Calibrator.hpp"
#include <fstream>
#include <string>
#include <cmath>
static double s_sig(double z){ return 1.0/(1.0+std::exp(-z)); }
bool Calibrator::load(const std::string& path){
    std::ifstream in(path);
    if(!in) return false;
    std::string k; double v;
    while(in>>k>>v){
        if(k=="w_pcyber") w_p=v;
        else if(k=="w_rule") w_r=v;
        else if(k=="w_novelty") w_n=v;
        else if(k=="bias") b=v;
    }
    return true;
}
double Calibrator::sig(double z){ return s_sig(z); }
double Calibrator::score(double pcyber, double rules, double novelty) const{
    return sig(w_p*pcyber + w_r*rules + w_n*novelty + b);
}
