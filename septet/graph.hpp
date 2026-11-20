#pragma once
#include "settings.hpp"
namespace septet_graph {
using namespace septet_eq;
#include "response.inc"
struct C { int re,im; };
// The table generator checks the worst-case products for all gains and points.
inline C times(C a, C b) {
    return {(a.re*b.re-a.im*b.im)>>12,(a.re*b.im+a.im*b.re)>>12};
}
inline int sweep_q12(int sweep) { return (sweep+4)>>3; }
inline int graph_scale(int value,int amount) { return (value*amount)>>12; }
inline int pixel(C d,int amount,int dry) {
    const int re=dry+graph_scale(d.re,amount), im=graph_scale(d.im,amount);
    if (re>=16384 || re<=-16384 || im>=16384 || im<=-16384) return 21;
    const int power=(re*re+im*im)>>8;
    int lo=0,hi=144;
    while(lo<hi){int m=(lo+hi)/2;if(db_power[m]<power)lo=m+1;else hi=m;}
    return 165-lo;
}
inline void plot(const Controls& p,int* side,int* mid) {
    unsigned active[7], count=0;
    for(unsigned j=0;j<7;++j) if(p.gain[j]) active[count++]=j;
    const int sweep=sweep_q12(p.sweep);
    const int feed=p.mode==SeriesParallel ? 4096-sweep : 4096;
    const int dry=p.mode==CorrectionSolo ? 4096-sweep : 4096;
    const int side_gain=p.mode==MidSide ? limit(2*sweep,0,4096) : 4096;
    const int mid_gain=p.mode==MidSide ? limit(2*(4096-sweep),0,4096) : side_gain;
    if (!count) {
        const int y=pixel({},0,dry);
        for(unsigned i=0;i<points;++i) side[i]=mid[i]=y;
        return;
    }
    for(unsigned i=0;i<points;++i){
        C sum={};
        for(unsigned k=0;k<count;++k){
            const unsigned j=active[k];
            const auto& e=correction[j][p.gain[j]+12][i];
            const C input=feed==4096 ? C{4096+sum.re,sum.im} :
                          feed==0 ? C{4096,0} :
                          C{4096+graph_scale(sum.re,feed),graph_scale(sum.im,feed)};
            const C delta=times({e[0],e[1]},input);
            sum.re+=delta.re;sum.im+=delta.im;
        }
        side[i]=pixel(sum,side_gain,dry);
        mid[i]=mid_gain==side_gain ? side[i] : pixel(sum,mid_gain,dry);
    }
}
}
