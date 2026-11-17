#pragma once
#include <cstddef>
namespace truce {
// Preserve both legacy contracts: direct SEL/NEXT/PREV choices and a cursor
// that moves with PREV/NEXT and accepts only on SEL. Merely opening never accepts.
class ChoiceState {
public:
    enum Key {Cancel,Accept,Next,Previous};
    static constexpr int Pending=-2;
    ChoiceState(size_t count,bool cycle):count_(count),cycle_(cycle){}
    int selected() const {return int(selected_);}
    int apply(Key key) {
        if(!count_)return 0;
        if(cycle_) {
            if(key==Accept)return int(selected_);
            if(key==Next)selected_=(selected_+1)%count_;
            else selected_=(selected_+count_-1)%count_;
            return Pending;
        }
        if(key==Cancel)return -1;
        if(key==Accept)return 0;
        if(key==Next && count_>1)return 1;
        if(key==Previous && count_>2)return 2;
        return Pending;
    }
private:
    size_t count_,selected_=0;
    bool cycle_;
};
}
