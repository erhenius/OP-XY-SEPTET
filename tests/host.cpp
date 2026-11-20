#include <initializer_list>
#include "eq.hpp"
#include "graph.hpp"
#include "graph_paths.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdint>
using namespace septet_eq;
using namespace septet_graph;
// Compare the actual integer DSP with the M3 display at all seven band centers,
// using steady-state stereo sinusoids, in-phase or opposite-polarity.
int main() {
    // Exercise 33-bit sums and differences on positive and negative coefficients.
    const i32 extrema[] = {-2147483647-1, -2147483647, -1, 0, 1, 2147483647};
    for (i32 x : extrema) for (i32 previous : extrema) for (int c : {-540000000, 540000000}) {
        assert(paired_product(c, x, previous, true) == i64(c) * (i64(x) + previous));
        assert(paired_product(c, x, previous, false) == i64(c) * (i64(x) - previous));
    }
    for (unsigned band=0; band<7; ++band) for (const auto& c : coefficient[band]) {
        if (band==0 || band==6) {
            assert(c[1]==(band==0 ? c[0] : -c[0]) && c[2]==0 && c[4]==0);
        } else {
            assert(c[1]==0 && c[2]==-c[0]);
        }
    }
    int word[4]={0,1073741824,2147483647,2147483647};
    auto p=controls(word,0);
    assert(p.gain[0]==-12 && p.gain[1]==0 && p.gain[2]==12 && p.sweep==32768);
    unsigned setting=0;
    for(unsigned j=0;j<4;++j)setting=edit(setting,j,int(j)*7-10);
    assert(gain_at(setting,0)==-10 && gain_at(setting,3)==11 && valid_setup(setting));
    assert(cycle(cycle(cycle(setting)))==setting);
    struct Vertex { int x,y; unsigned type; };
    Vertex contour[contour_commands]={};
    contour[0]={31,168*1024,1};
    for(unsigned i=0;i<points;++i) contour[i+1]={px[i],68*1024,2};
    contour[points+1]={449,168*1024,2};
    contour[points+2].type=5;
    Vertex ribbon[ribbon_commands]={};
    for (unsigned i=0;i<2*points;++i)
        ribbon[i].type=i==0 ? 1 : 2;
    ribbon[2*points].type=5;
    assert(valid_polygon(contour,contour_commands) && valid_polygon(ribbon,ribbon_commands));
    int sampled_side[points],sampled_mid[points];
    plot(p,sampled_side,sampled_mid);
    write_contour(contour,sampled_side,-1);
    write_ribbon(ribbon,sampled_mid,1);
    assert(contour[0].y==168*1024 && contour[points+1].y==168*1024);
    assert(contour[1].y!=68*1024);
    for(unsigned i=0;i<points;++i)
        assert(contour[i+1].x==px[i] && contour[i+1].y==(sampled_side[i]-1)*1024);
    for(unsigned i=0;i<points;++i) {
        assert(ribbon[i].y==(sampled_mid[i]-1)*1024);
        assert(ribbon[2*points-1-i].y==(sampled_mid[i]+1)*1024);
    }
    // Stress both the checked product bounds and clipped dB conversion.
    for(unsigned combination=0;combination<2187;++combination) {
        unsigned n=combination;
        for(auto& gain:p.gain){gain=(int(n%3)-1)*12;n/=3;}
        for(auto mode:{SeriesParallel,CorrectionSolo,MidSide}) for(int sweep:{0,16384,32768}){
            p.mode=mode;p.sweep=sweep;
            plot(p,sampled_side,sampled_mid);
            for(unsigned i=0;i<points;++i) {
                assert(sampled_side[i]>=21 && sampled_side[i]<=165);
                assert(sampled_mid[i]>=21 && sampled_mid[i]<=165);
            }
        }
    }
    int freq[]={80,160,320,640,1250,2500,8000};
    int gains[]={4,1,-8,-6,2,5,1};
    for(unsigned j=0;j<7;++j)p.gain[j]=gains[j];
    int checks=0;
    for(auto mode:{SeriesParallel,CorrectionSolo,MidSide}) for(int pos:{0,16384,32768})
        for(int phase:{-1,1}) for(unsigned i=0;i<7;++i){
            Bank bank={};p.mode=mode;p.sweep=pos;
            int side[points],mid[points];plot(p,side,mid);
            double in=0,out=0;
            for(int block=0;block<240;++block){
                int buf[128]; double before[64];
                for(int n=0;n<64;++n){buf[2*n]=int(8e7*std::sin(2*3.141592653589793*freq[i]*(64*block+n)/44100.));
                    before[n]=buf[2*n];buf[2*n+1]=phase*buf[2*n];}
                process(bank,p,buf,64);
                if(block>=100)for(int n=0;n<64;++n){in+=before[n]*before[n];out+=double(buf[2*n])*buf[2*n];}
            }
            double db=10*std::log10(out/in);
            double graph=(69-(mode==MidSide && phase>0?mid:side)[dot_index[i]])*.25;
            if(std::abs(std::fmax(-24.,std::fmin(12.,db))-graph)>.43){
                std::printf("mismatch %d %d %d %d: %.2f %.2f\n",mode,pos,phase,freq[i],db,graph);return 1;}
            ++checks;
        }
    Bank b={};Controls flat={};int buf[128];
    for(int i=0;i<128;++i)buf[i]=i*271-50000;
    process(b,flat,buf,64); for(int i=0;i<128;++i)assert(buf[i]==i*271-50000);
    flat.mode=CorrectionSolo;flat.sweep=32768;
    for(int j=0;j<4;++j){for(int i=0;i<128;++i)buf[i]=i*271-50000;process(b,flat,buf,64);}
    for(int n:buf)assert(!n);
    for (int gain : {-12,12}) {
        Bank tail={};Controls shelf={};shelf.gain[0]=gain;
        for(int block=0;block<1000;++block){
            for(auto& x:buf)x=0;
            if(block==1)buf[0]=1000000000;
            process(tail,shelf,buf,64);
            if(block>800)for(auto x:buf)assert(x==0);
        }
    }
    std::uint32_t random=17;
    for(int block=0;block<2400;++block){
        random=random*1664525+1013904223;
        for(unsigned j=0;j<7;++j)flat.gain[j]=((random>>j)&1)?12:-12;
        flat.mode=Mode((block/17)%3);flat.sweep=random%32769;
        for(auto& n:buf){random=random*1664525+1013904223;n=int(random);}
        process(b,flat,buf,64);
    }
    std::printf("PASS %d swept response comparisons, control mapping, neutral, delta null and transition stress\n",checks);
}
