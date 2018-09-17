//
// Created by joro on 9/16/18.
//

#include "ZoneManager.h"
#include "proto/dawnmud.pb.h"


// Zone implementation

Zone::Zone() { }

Zone::Zone(const DawnMud::Zone & _zone) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    name_ = _zone.name();
    for (int i = 0; i < _zone.builders_size(); i++) {
        builders_.push_back(_zone.builders(i));
    }
    lifespan_ = _zone.lifespan();
    age_ = _zone.age();
    bot_ = _zone.bot();
    top_ = _zone.top();
    reset_mode_ = convert(_zone.reset_mode());
    number_ = _zone.number();

    for (int i = 0; i < _zone.reset_com_size(); i++) {
        add_command(_zone.reset_com(i));

    }

}

Zone::Zone(DawnMud::Zone &_zone) : Zone(const_cast<const DawnMud::Zone &>(_zone)) { }

Zone::~Zone() {}

// Getters
std::string & Zone::get_name() { return name_; }


// Setters
void Zone::set_name(std::string _name) { name_ = _name; }
void Zone::set_builders(std::vector <std::string> _builders) {
    for (int i = 0; i < _builders.size(); i++) {
        builders_.push_back(_builders[i]);
    }
}
void Zone::set_lifespan(int _lifespan) { lifespan_ = _lifespan; }
void Zone::set_age(int _age) { age_ = _age; }
void Zone::set_bot(room_vnum _bottom) { bot_ = _bottom; }
void Zone::set_top(room_vnum _top) { top_ = _top; }
void Zone::set_reset(Reset _reset) { reset_mode_ = _reset; }
void Zone::set_reset(DawnMud::Zone::ResetMode _reset) { reset_mode_ = convert(_reset); }
void Zone::set_number(zone_vnum _number) { number_ = _number; }
void Zone::add_command(ResetCommandType _command, bool _if_flag, int _arg1, int _arg2, int _arg3, int _line) {
    struct ResetCommand _cmd;
    _cmd.reset_command = _command;
    _cmd.if_flag = _if_flag;
    _cmd.arg1 = _arg1;
    _cmd.arg2 = _arg2;
    _cmd.arg3 = _arg3;
    _cmd.line = _line;

    rst_cmds_.push_back(_cmd);
}

void Zone::add_command(const DawnMud::Zone::ResetCommands &_cmd) {
    add_command(convert(_cmd.command()), _cmd.if_flag(), _cmd.arg1(), _cmd.arg2(), _cmd.arg3(), _cmd.line());
}

// Private conversion
Zone::Reset Zone::convert(DawnMud::Zone::ResetMode _reset) {
    switch(_reset){
        case DawnMud::Zone::NO_RESET:
            return kNoReset;
        case DawnMud::Zone::RESET:
            return kReset;
        case DawnMud::Zone::HARD_RESET:
            return kHardReset;
        default:
            // TODO Add logging here for error
            return kResetError;
    }
}

DawnMud::Zone::ResetMode Zone::convert(Zone::Reset _reset) {
    switch(_reset) {
        case kNoReset:
            return DawnMud::Zone::NO_RESET;
        case kReset:
            return DawnMud::Zone::RESET;
        case kHardReset:
            return DawnMud::Zone::HARD_RESET;
        default:
            // TODO add logging here for error
            return DawnMud::Zone::HARD_RESET;
    }
}

Zone::ResetCommandType Zone::convert(DawnMud::Zone::ResetCommands::Commands _reset) {
    switch(_reset) {
        case DawnMud::Zone::ResetCommands::READ_MOBILE:
            return kReadMobile;
        case DawnMud::Zone::ResetCommands::READ_OBJECT:
            return kReadObject;
        case DawnMud::Zone::ResetCommands::GIVE_OBJ_MOB:
            return kGiveObjToMob;
        case DawnMud::Zone::ResetCommands::PUT_OBJ_OBJ:
            return kPutObjInObj;
        case DawnMud::Zone::ResetCommands::GIVE_OBJ_CHAR:
            return kGiveObjToChar;
        case DawnMud::Zone::ResetCommands::OBJ_CHAR_EQUIP:
            return kObjCharEquip;
        case DawnMud::Zone::ResetCommands::STATE_OF_DOOR:
            return kStateOfDoor;
        default:
            // TODO Log error here
            return kResetCommandTypeError;
    }
}

DawnMud::Zone::ResetCommands::Commands Zone::convert(Zone::ResetCommandType _reset) {
    switch (_reset) {
        case kReadMobile:
            return DawnMud::Zone::ResetCommands::READ_MOBILE;
        case kReadObject:
            return DawnMud::Zone::ResetCommands::READ_OBJECT;
        case kGiveObjToMob:
            return DawnMud::Zone::ResetCommands::GIVE_OBJ_MOB;
        case kPutObjInObj:
            return DawnMud::Zone::ResetCommands::PUT_OBJ_OBJ;
        case kGiveObjToChar:
            return DawnMud::Zone::ResetCommands::GIVE_OBJ_CHAR;
        case kObjCharEquip:
            return DawnMud::Zone::ResetCommands::OBJ_CHAR_EQUIP;
        case kStateOfDoor:
            return DawnMud::Zone::ResetCommands::STATE_OF_DOOR;
        default:
            // TODO log error
            return DawnMud::Zone::ResetCommands::READ_MOBILE;
    }
}


/*
 *
 * ZoneManager implementation
 *
 */
int ZoneManager::populate_zones() {
    return populate_zones(zone_path_);
}

int ZoneManager::populate_zones(std::string _zone_path) {
    // Make sure zones is empty, or otherwise empty it and re-populate
    if (zones_.size() != 0) {
        zones_.empty();
    }

    // Read in the zones from the serialized file
    DawnMud::ZoneList zone_list;
    std::fstream input(_zone_path, std::ios::in | std::ios::binary);
    if (!input) {
        std::cout << "Could not find the file " << _zone_path << std::endl;
    } else if (!zone_list.ParseFromIstream(&input)) {
        std::cerr << "Failed to parse the zone file... " << std::endl;
        return -1;
    }

    // Populate the zones internally
    for (int i = 0; i < zone_list.zone_size(); i++) {
        Zone _z(zone_list.zone(i));
        zones_.push_back(_z);
    }

    // Return zone number
    return zones_.size();
};

void ZoneManager::test_zone_manager() {
    std::cout << "Testing" << std::endl;
}