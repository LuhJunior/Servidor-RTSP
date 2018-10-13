#include <iostream>
#include "ServerRTP.hpp"

int main(){
    Server HTTP;
    HTTP.mylisten(PORT);
    HTTP.myclose();
    return 0;
}