#pragma once
#include <cstdint>
namespace truce {
inline bool validTouch(int count,bool pressed,int x,int y,int width,int height) {
    return count>0 && pressed && x>=0 && y>=0 && x<width && y<height;
}
// Fast touch polling must not erase a key before a slower screen consumes it.
class NavigationLatch {
    bool armed_=false;
    uint32_t since_=0;
public:
    bool retain(bool active,bool pending,uint32_t now) {
        if(!active || !pending){armed_=false;return false;}
        if(!armed_){armed_=true;since_=now;}
        if(uint32_t(now-since_)<150)return true;
        armed_=false;return false;
    }
};
}
