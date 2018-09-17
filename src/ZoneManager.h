//
// Created by joro on 9/16/18.
//

#ifndef CIRCLEMUD_ZONEMANAGER_H
#define CIRCLEMUD_ZONEMANAGER_H

//#include "db.hpp"
#include <vector>
#include <iostream>
#include <fstream>
#include "proto/dawnmud.pb.h"

typedef int room_vnum;
typedef int zone_vnum;


class Zone {
public:
    // Constructors
    Zone();
    Zone(DawnMud::Zone &_zone);
    Zone(const DawnMud::Zone &_zone);

    // Destructor
    ~Zone();

    // Support
    enum Reset {
        /*
         * Reset mode:
         *   0: Don't reset, and don't update age.
         *   1: Reset if no PC's are located in zone.
         *   2: Just reset.
         */
        kNoReset = 0,
        kReset = 1,
        kHardReset = 2,
        kResetError = 9
    };

    enum ResetCommandType {
        kReadMobile = 0,
        kReadObject = 1,
        kGiveObjToMob = 2,
        kPutObjInObj = 3,
        kGiveObjToChar = 4,
        kObjCharEquip = 5,
        kStateOfDoor = 6,
        kResetCommandTypeError = 9
    };

    struct ResetCommand {
        ResetCommandType reset_command;
        bool if_flag;
        int arg1;
        int arg2;
        int arg3;
        int line;
    };

    // Getters
    std::string &get_name();

    // Setters
    void set_name(std::string _name);
    void set_builders(std::vector<std::string> _builders);
    void set_lifespan(int _lifespan);
    void set_age(int _age);
    void set_bot(room_vnum _bottom);
    void set_top(room_vnum _top);
    void set_reset(Reset _reset);
    void set_reset(DawnMud::Zone::ResetMode _reset);
    void set_number(zone_vnum _number);
    void add_command(ResetCommandType _command, bool _if_flag, int _arg1, int _arg2, int _arg3, int _line);
    void add_command(const DawnMud::Zone::ResetCommands &_cmd);

private:


    std::string name_;                          /* name of this zone                    */
    std::vector<std::string> builders_;        /* namelist of builders allowed to      */
    /* modify this zone.		            */
    int lifespan_;                              /* how long between resets (minutes)    */
    int age_;                                   /* current age of this zone (minutes)   */
    room_vnum bot_;                             /* starting room number for this zone   */
    room_vnum top_;                             /* upper limit for rooms in this zone   */
    Reset reset_mode_;                          /* conditions for reset (see below)     */
    zone_vnum number_;                          /* virtual number of this zone	        */



    // TODO Feels like a template could be used here somehow...
    Reset convert(DawnMud::Zone::ResetMode _reset);
    DawnMud::Zone::ResetMode convert (Reset _reset);
    ResetCommandType convert(DawnMud::Zone::ResetCommands::Commands _reset);
    DawnMud::Zone::ResetCommands::Commands convert(ResetCommandType _reset);

    std::vector <struct ResetCommand> rst_cmds_;/* command table for reset	            */

};


class ZoneManager {
public:

    std::vector<Zone> zones_;
    std::string zone_path_;

    ZoneManager() : zone_path_("lib/world/zon") {};

    ~ZoneManager() {};

    // Management functions
    int populate_zones();
    int populate_zones(std::string _zone_path);

    // Debug
    void test_zone_manager();


};

#endif //CIRCLEMUD_ZONEMANAGER_H
