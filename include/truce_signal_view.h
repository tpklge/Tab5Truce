#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
namespace truce {
// Presentation only: resample an existing capture; never acquire radio data.
template<class Sample>
void signalEnvelope(uint8_t *out, size_t count, Sample sample, int64_t minimum, int64_t maximum) {
    for(size_t i=0;i<256;++i) {
        if(!count || maximum<=minimum){out[i]=0;continue;}
        const int64_t value=sample(i*(count-1)/255);
        out[i]=uint8_t((std::max(minimum,std::min(maximum,value))-minimum)*100/(maximum-minimum));
    }
}
inline void digitalEnvelope(uint8_t *out,const std::vector<int> &capture,int start,int width,int divider) {
    std::fill(out,out+256,0);
    if(capture.empty() || capture[0]==0 || width<=0 || divider<=0)return;
    auto duration=[&](size_t i){int64_t us=capture[i];if(us<0)us=-us;return std::min<int64_t>(20000,us)/divider;};
    size_t segment=0;int64_t end=duration(0);
    for(int i=0;i<256;++i) {
        const int64_t position=int64_t(start)+int64_t(i)*width/255;
        while(position>=end && segment+1<capture.size()) {
            ++segment;
            if(segment%2==0 && capture[segment]==0)return;
            end+=duration(segment);
        }
        if(position<end && segment%2==0)out[i]=100;
    }
}
}
