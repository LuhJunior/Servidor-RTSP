#include <iostream>
#include "ServerRTP.hpp"

int main(){
    while(true){
        /*
        Server rtp, rtp2;
        _beginthreadex( nullptr, 0, &listenThread, (void *) &rtp, 0, nullptr);
        rtp2.porta = 9000;
        _beginthreadex( nullptr, 0, &listenThread, (void *) &rtp2, 0, nullptr);
        */
        Server rtp;
        rtp.mylisten(PORT);
        rtp.myclose();
    }
    return 0;
}