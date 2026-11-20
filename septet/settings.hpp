#pragma once
#include "eq.hpp"
namespace septet_eq {
// Zero is a neutral project. The four signed gain fields use sign/magnitude.
constexpr unsigned setup_bits=22, setup_mask=(1u<<setup_bits)-1;
constexpr unsigned extra_gain_mask=(1u<<20)-1;
inline int gain_at(unsigned v,unsigned j) {
    unsigned f=(v>>(5*j))&31;
    return (f&16) ? -int(f&15) : int(f&15);
}
inline bool valid_setup(unsigned v) {
    if(v>setup_mask || (v>>20)>2) return false;
    for(unsigned j=0;j<4;++j) if(((v>>(5*j))&15)>12) return false;
    return true;
}
inline unsigned edit(unsigned v,unsigned j,int delta) {
    const int gain=limit(gain_at(v,j)+limit(delta,-24,24),-12,12);
    unsigned f=gain<0 ? unsigned(16-gain) : unsigned(gain);
    return (v&~(31u<<(5*j)))|(f<<(5*j));
}
inline unsigned cycle(unsigned v) { return (v&extra_gain_mask)|((((v>>20)+1)%3)<<20); }
inline Controls controls(const i32* words,unsigned setup) {
    Controls p={};
    for(unsigned j=0;j<3;++j) {
        unsigned w=words[j]>0 ? unsigned(words[j]) : 0;
        p.gain[j]=int((((w>>15)*24u)+32768)>>16)-12;
    }
    for(unsigned j=0;j<4;++j) p.gain[j+3]=gain_at(setup,j);
    p.mode=Mode(setup>>20);
    p.sweep=int(((words[3]>0 ? unsigned(words[3]) : 0)+32768u)>>16);
    return p;
}
}
