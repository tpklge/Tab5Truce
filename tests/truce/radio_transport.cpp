#include <Arduino.h>
#include <truce_radio_policy.h>
#include <cassert>
#include <cstring>
static bool ready=false;
static int calls=0;
bool hostedInitBLE(){++calls;return ready;}
int main(){
    assert(!truce::prepareHostedBle());assert(calls==1);
    assert(strstr(truce::hostedBleFailure(),"C6"));
    ready=true;
    assert(truce::prepareHostedBle());assert(calls==2);
    assert(truce::prepareHostedBle());assert(calls==2);
    assert(strstr(truce::hostedBleFailure(),"Memória"));
}
