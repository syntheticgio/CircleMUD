//
// Created by joro on 9/16/18.
//

#ifndef CIRCLEMUD_GAMEMANAGER_H
#define CIRCLEMUD_GAMEMANAGER_H

#include "ZoneManager.h"
#include "proto/dawnmud.pb.h"


class GameManager {

public:
    GameManager();
    ~GameManager();

    /**
     * Initialize the MUD.
     * @return
     */
    int init_mud();

public:
    // Prevent copying of a GameManager instance
    // TODO I probably don't really need to do this...
    GameManager(GameManager const &) = delete;
    GameManager &operator=(GameManager const &) = delete;

private:
    ZoneManager zone_manager_;
    int populate_zones();

};


#endif //CIRCLEMUD_GAMEMANAGER_H
