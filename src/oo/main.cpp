//
// Created by joro on 9/16/18.
//

#include "../GameManager.h"
#include <iostream>


int main (int argc, char ** argv ) {

    std::cout << "Attempting to run..." << std::endl;
    GameManager x;
    int error = x.init_mud();
    if (error < 0) {
        std::cout << "Exiting program due to error." << std::endl;
        return -1;
    }

    return 0;

}