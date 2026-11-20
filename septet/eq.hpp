#pragma once
#include <opxy/q31.hpp>

namespace septet_eq {
using namespace opxy;
enum Mode { SeriesParallel, CorrectionSolo, MidSide };
struct Controls { int gain[7]; Mode mode; int sweep; }; // integer dB, sweep 0..32768
#include "coefficients.inc"

inline int limit(int n, int lo, int hi) { return n < lo ? lo : n > hi ? hi : n; }
inline i32 sat(i64 x) {
    return x > 2147483647 ? 2147483647 : x < -2147483647-1 ? -2147483647-1 : i32(x);
}
inline i32 scale(i32 sample, int amount) { return i32(q31::wide(sample,amount) >> 15); }
struct History { i32 x1,x2,d1,d2,residue; };
struct Bank {
    History h[2][7];
    Controls current;
    int wet;
    bool started;
};

// Fold equal numerator coefficients before the wide multiply. A pair of
// full-scale inputs can need 33 bits. Restore the carry on that rare path.
inline i64 paired_product(int coefficient, i32 x, i32 earlier, bool sum) {
    i32 pair;
    bool overflow = sum ? __builtin_add_overflow(x, earlier, &pair)
                        : __builtin_sub_overflow(x, earlier, &pair);
    i64 product = q31::wide(coefficient, pair);
    if (overflow) {
        const i64 carry = i64(coefficient) * 4294967296LL;
        product += x < 0 ? -carry : carry;
    }
    return product;
}

inline i32 correction_sample(History& h, const int* c, unsigned band, i32 x) {
    const bool bell = band > 0 && band < 6;
    i64 v = paired_product(c[0], x, bell ? h.x2 : h.x1, band == 0)
          + q31::wide(c[3], h.d1);
    if (bell) v += q31::wide(c[4], h.d2);
    // Return the discarded fraction to the next sample so a bass shelf
    // decays into silence instead of settling on a DC limit cycle.
    v += h.residue;
    i32 d = sat((v + (1<<27)) >> 28);
    h.residue = (d == 2147483647 || d == -2147483647-1) ? 0 : i32(v-i64(d)*(1<<28));
    h.x2=h.x1; h.x1=x; h.d2=h.d1; h.d1=d;
    return d;
}
inline void process(Bank& bank, const Controls& target, i32* stereo, unsigned frames) {
    if (!frames) return;
    if (!bank.started) { bank.current=target; bank.started=true; }
    if (bank.current.mode != target.mode && !bank.wet) {
        for (auto& channel : bank.h) for (auto& h : channel) h={};
        bank.current=target;
    }
    const bool switching=bank.current.mode!=target.mode;
    const Mode mode=bank.current.mode;
    const int target_sweep=limit(target.sweep,0,32768);
    const int sweep_start=bank.current.sweep;
    unsigned active[7], count=0;
    for (unsigned j=0;j<7;++j) {
        int& g=bank.current.gain[j];
        int goal=limit(target.gain[j],-12,12);
        g+= (goal>g)-(goal<g);
        if (g) active[count++]=j;
        else { bank.h[0][j]={}; bank.h[1][j]={}; }
    }
    if (!count && mode!=CorrectionSolo && !switching) {
        bank.current.sweep=target_sweep;
        bank.wet=32768;
        return;
    }
    // Firmware blocks are 64 frames. Interpolate controls once per stereo frame.
    int sweep=sweep_start;
    const bool moving = target_sweep != sweep_start;
    const int increment = moving ? (target_sweep-sweep_start)/int(frames) : 0;
    for(unsigned n=0;n<frames;++n) {
        if (moving) sweep=n+1==frames ? target_sweep : sweep+increment;
        if (switching || bank.wet < 32768)
            bank.wet=limit(bank.wet+(switching?-512:512),0,32768);
        if (!count && mode!=CorrectionSolo) continue;
        const i32 original[2]={stereo[2*n],stereo[2*n+1]};
        const i32 l=original[0]>>6, r=original[1]>>6;
        i32 x[2]={l,r};
        if (mode==MidSide) { x[0]=(l+r)/2; x[1]=(l-r)/2; }
        i32 delta[2]={};
        const int feedback = mode == SeriesParallel ? 32768-sweep : 32768;
        for (unsigned k=0;k<count;++k) {
            const unsigned j=active[k];
            const int* c=coefficient[j][bank.current.gain[j]+12];
            for (unsigned ch=0;ch<2;++ch) {
                const i32 feed = feedback == 32768 ? delta[ch] :
                                 feedback == 0 ? 0 : scale(delta[ch], feedback);
                delta[ch]=q31::add(delta[ch],correction_sample(bank.h[ch][j],c,j,q31::add(x[ch],feed)));
            }
        }
        if (mode==MidSide) {
            const i32 m=scale(delta[0],limit(2*(32768-sweep),0,32768));
            const i32 s=scale(delta[1],limit(2*sweep,0,32768));
            delta[0]=sat(i64(m)+s); delta[1]=sat(i64(m)-s);
        }
        for(unsigned ch=0;ch<2;++ch) {
            const i32 removed = mode==CorrectionSolo ? scale(original[ch],sweep) : 0;
            const bool full = bank.wet == 32768;
            const i32 correction = full ? delta[ch] : scale(delta[ch],bank.wet);
            stereo[2*n+ch]=sat(i64(original[ch])-(full ? removed : scale(removed,bank.wet))+i64(correction)*64);
        }
    }
    bank.current.sweep=target_sweep;
}
}
