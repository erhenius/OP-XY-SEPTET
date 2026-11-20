#pragma once
#include "graph.hpp"

namespace septet_graph {
constexpr unsigned contour_commands=points+3;
constexpr unsigned ribbon_commands=2*points+1;

// Every dynamic shape in the artwork is a single closed M/L/Z polygon.
template<class Command>
bool valid_polygon(const Command* path,unsigned count) {
    if (path[0].type!=1 || path[count-1].type!=5) return false;
    for (unsigned i=1;i+1<count;++i)
        if (path[i].type!=2) return false;
    return true;
}

template<class Command>
void write_contour(Command* path,const int* y,int inset) {
    for (unsigned i=0;i<points;++i) path[i+1].y=(y[i]+inset)*1024;
}
template<class Command>
void write_ribbon(Command* path,const int* y,int lower_edge) {
    for (unsigned i=0;i<points;++i) {
        path[i].y=(y[i]-1)*1024;
        path[2*points-1-i].y=(y[i]+lower_edge)*1024;
    }
}
}
