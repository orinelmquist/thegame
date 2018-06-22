//
//  main.cpp
//  GameProject
//
//  Created by Orin Elmquist on 6/12/18.
//  Copyright © 2018 Orin Elmquist. All rights reserved.
//

#include <iostream>
#include <stdlib.h>
#include <time.h>
#include "Game.hpp"

//const unsigned int SEED = 143245543;

//Seed         Error
//1529537124   Unconnected part of cave

int main() {
        
//    unsigned int seed = time(NULL);
    unsigned int seed = 1529537124;

    srand(seed);

    std::cout << seed << std::endl << std::endl;

    World w;

    w.buildCave();

    std::cout << w << std::endl;

    w.buildDungeon();

    std::cout << w << std::endl;
    
    return 0;
}
