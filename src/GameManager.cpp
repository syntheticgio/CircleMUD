//
// Created by joro on 9/16/18.
//

#include "GameManager.h"

GameManager::GameManager() {
    std::cout << "In the Game Manager constructor.." << std::endl;
}

GameManager::~GameManager() {

}

int GameManager::init_mud() {

    // Things to populate
//      DB_BOOT_WLD:
//      DB_BOOT_MOB:
//      DB_BOOT_OBJ:
//    x DB_BOOT_ZON:
//      DB_BOOT_SHP:
//      DB_BOOT_HLP:

    // Populate zones
    int error = populate_zones();
    if (error < 0) {
        std::cout << "Failed to parse the zone file..." << std::endl;
        return error;
    } else {
        std::cout << "Zones loaded.  Zone number: " << error << std::endl;
    }

    // Populate Mobs
}

int GameManager::populate_zones() {
    return zone_manager_.populate_zones("../../python_gui/test");
}