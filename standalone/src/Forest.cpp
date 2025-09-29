#include "Forest.hpp"
#include <fstream>
#include <sstream>
std::vector<double> Forest::proba(const std::vector<double>& x) const{
    double a0=0,a1=0,a2=0;
    for(const auto& t:trees){
        int i=0;
        while(!t.n[i].leaf){
            const auto& nd = t.n[i];
            i = (x[nd.f] <= nd.t) ? nd.l : nd.r;
        }
        a0+=t.n[i].p[0]; a1+=t.n[i].p[1]; a2+=t.n[i].p[2];
    }
    double Z = a0+a1+a2;
    if(Z<=0) return {0.34,0.33,0.33};
    return {a0/Z, a1/Z, a2/Z};
}
bool Forest::load(const std::string& path){
    std::ifstream in(path);
    if(!in) return false;
    trees.clear();
    std::string tag; int T=0;
    in>>tag>>T;
    for(int t=0;t<T;++t){
        std::string tt; int N; in>>tt>>N;
        ForestTree tr; tr.n.resize(N);
        for(int i=0;i<N;++i){
            int idx,f,l,r; double th,p0,p1,p2;
            in>>idx>>f>>th>>l>>r>>p0>>p1>>p2;
            tr.n[i].f=f; tr.n[i].t=th; tr.n[i].l=l; tr.n[i].r=r;
            tr.n[i].leaf = (l<0 && r<0);
            tr.n[i].p[0]=p0; tr.n[i].p[1]=p1; tr.n[i].p[2]=p2;
        }
        trees.push_back(tr);
    }
    return true;
}
