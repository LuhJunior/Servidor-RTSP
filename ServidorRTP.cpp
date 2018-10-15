#include <iostream>
#include "ServerRTP.hpp"

int main(){
    Server rtp;
    rtp.mylisten(PORT);
    rtp.myclose();
    return 0;
}