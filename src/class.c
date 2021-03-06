/* ************************************************************************
*   File: class.c                                       Part of CircleMUD *
*  Usage: Source file for class-specific code                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */



#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "db.h"
#include "utils.h"
#include "spells.h"
#include "interpreter.h"
#include "constants.h"

/* local functions */
void snoop_check(struct char_data *ch);

int parse_class(char arg);

bitvector_t find_class_bitvector(const char *arg);

byte saving_throws(int class_num, int type, int level);

int thaco(int class_num, int level);

void roll_real_abils(struct char_data *ch);

void do_start(struct char_data *ch);

int backstab_mult(int level);

int invalid_class(struct char_data *ch, struct obj_data *obj);

int level_exp(int chclass, int level);

const char *title_male(int chclass, int level);

const char *title_female(int chclass, int level);

/* Names first */

const char *class_abbrevs[] = {
        "Mu",
        "Cl",
        "Th",
        "Wa",
        "Ra",
        "\n"
};


const char *pc_class_types[] = {
        "Magic User",
        "Cleric",
        "Thief",
        "Warrior",
        "Ranger",
        "\n"
};


/* The menu for choosing a class in interpreter.c: */
const char *class_menu =
        "\r\n"
                "Select a class:\r\n"
                "  [C]leric\r\n"
                "  [T]hief\r\n"
                "  [W]arrior\r\n"
                "  [M]agic-user\r\n"
                "  [R]anger\r\n";


/*
 * The code to interpret a class letter -- used in interpreter.c when a
 * new character is selecting a class and by 'set class' in act.wizard.c.
 */

int parse_class(char arg) {
    arg = LOWER(arg);

    switch (arg) {
        case 'm':
            return CLASS_MAGIC_USER;
        case 'c':
            return CLASS_CLERIC;
        case 'w':
            return CLASS_WARRIOR;
        case 't':
            return CLASS_THIEF;
        case 'r':
            return CLASS_RANGER;
        default:
            return CLASS_UNDEFINED;
    }
}

/*
 * bitvectors (i.e., powers of two) for each class, mainly for use in
 * do_who and do_users.  Add new classes at the end so that all classes
 * use sequential powers of two (1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4,
 * 1 << 5, etc.) up to the limit of your bitvector_t, typically 0-31.
 */
bitvector_t find_class_bitvector(const char *arg) {
    size_t rpos, ret = 0;

    for (rpos = 0; rpos < strlen(arg); rpos++)
        ret |= (1 << parse_class(arg[rpos]));

    return (ret);
}


/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 *
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 *
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SPELL    0
#define SKILL    1

/* #define LEARNED_LEVEL	0  % known which is considered "learned" */
/* #define MAX_PER_PRAC		1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC		2  min percent gain in skill per practice */
/* #define PRAC_TYPE		3  should it say 'spell' or 'skill'?	*/

int prac_params[4][NUM_CLASSES] = {
        /* MAG	CLE	THE	WAR RAN */
        {95,  95,  85, 80, 85},    /* learned level */
        {100, 100, 12, 12, 25},    /* max per practice */
        {25,  25,  0,  0,  0},    /* min per practice */
        {SPELL, SPELL, SKILL, SKILL, SKILL},    /* prac name */
};


/*
 * ...And the appropriate rooms for each guildmaster/guildguard; controls
 * which types of people the various guildguards let through.  i.e., the
 * first line shows that from room 3017, only MAGIC_USERS are allowed
 * to go south.
 *
 * Don't forget to visit spec_assign.c if you create any new mobiles that
 * should be a guild master or guard so they can act appropriately. If you
 * "recycle" the existing mobs that are used in other guilds for your new
 * guild, then you don't have to change that file, only here.
 */
struct guild_info_type guild_info[] = {

/* Midgaard */
        {CLASS_MAGIC_USER, 3017, SCMD_SOUTH},
        {CLASS_CLERIC,     3004, SCMD_NORTH},
        {CLASS_THIEF,      3027, SCMD_EAST},
        {CLASS_WARRIOR,    3021, SCMD_EAST},
        {CLASS_RANGER,     3021, SCMD_EAST},

/* Brass Dragon */
        {-999 /* all */ ,  5065, SCMD_WEST},

/* this must go last -- add new guards above! */
        {-1, NOWHERE, -1}
};


/*
 * Saving throws for:
 * MCTW
 *   PARA, ROD, PETRI, BREATH, SPELL
 *     Levels 0-40
 *
 * Do not forget to change extern declaration in magic.c if you add to this.
 */

byte saving_throws(int class_num, int type, int level) {
    // TODO: Change this to regression formula instead of raw values (or read in from external table)
    switch (class_num) {
        case CLASS_MAGIC_USER:
            switch (type) {
                case SAVING_PARA:    /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 70;
                        case 2:
                            return 69;
                        case 3:
                            return 68;
                        case 4:
                            return 67;
                        case 5:
                            return 66;
                        case 6:
                            return 65;
                        case 7:
                            return 63;
                        case 8:
                            return 61;
                        case 9:
                            return 60;
                        case 10:
                            return 59;
                        case 11:
                            return 57;
                        case 12:
                            return 55;
                        case 13:
                            return 54;
                        case 14:
                            return 53;
                        case 15:
                            return 53;
                        case 16:
                            return 52;
                        case 17:
                            return 51;
                        case 18:
                            return 50;
                        case 19:
                            return 48;
                        case 20:
                            return 46;
                        case 21:
                            return 45;
                        case 22:
                            return 44;
                        case 23:
                            return 42;
                        case 24:
                            return 40;
                        case 25:
                            return 38;
                        case 26:
                            return 36;
                        case 27:
                            return 34;
                        case 28:
                            return 32;
                        case 29:
                            return 30;
                        case 30:
                            return 28;
                        case 31:
                            return 26;
                        case 32:
                            return 24;
                        case 33:
                            return 22;
                        case 34:
                            return 20;
                        case 35:
                            return 18;
                        case 36:
                            return 17;
                        case 37:
                            return 16;
                        case 38:
                            return 15;
                        case 39:
                            return 14;
                        case 40:
                            return 12;
                        case 41:
                            return 11;
                        case 42:
                            return 10;
                        case 43:
                            return 9;
                        case 44:
                            return 8;
                        case 45:
                            return 7;
                        case 46:
                            return 6;
                        case 47:
                            return 5;
                        case 48:
                            return 3;
                        case 49:
                            return 2;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 49)
                        return 0;
                    else
                        log("SYSERR: Missing level for mage paralyzation saving throw.");
                    break;
                case SAVING_ROD:    /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 55;
                        case 2:
                            return 53;
                        case 3:
                            return 51;
                        case 4:
                            return 49;
                        case 5:
                            return 47;
                        case 6:
                            return 45;
                        case 7:
                            return 43;
                        case 8:
                            return 41;
                        case 9:
                            return 40;
                        case 10:
                            return 39;
                        case 11:
                            return 37;
                        case 12:
                            return 35;
                        case 13:
                            return 33;
                        case 14:
                            return 31;
                        case 15:
                            return 30;
                        case 16:
                            return 29;
                        case 17:
                            return 27;
                        case 18:
                            return 25;
                        case 19:
                            return 23;
                        case 20:
                            return 21;
                        case 21:
                            return 20;
                        case 22:
                            return 19;
                        case 23:
                            return 17;
                        case 24:
                            return 15;
                        case 25:
                            return 14;
                        case 26:
                            return 13;
                        case 27:
                            return 12;
                        case 28:
                            return 11;
                        case 29:
                            return 10;
                        case 30:
                            return 9;
                        case 31:
                            return 8;
                        case 32:
                            return 7;
                        case 33:
                            return 6;
                        case 34:
                            return 5;
                        case 35:
                            return 4;
                        case 36:
                            return 3;
                        case 37:
                            return 2;
                        case 38:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 38)
                        return 0;
                    else
                        log("SYSERR: Missing level for mage rods saving throw.");
                    break;
                case SAVING_PETRI:    /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 65;
                        case 2:
                            return 63;
                        case 3:
                            return 61;
                        case 4:
                            return 59;
                        case 5:
                            return 57;
                        case 6:
                            return 55;
                        case 7:
                            return 53;
                        case 8:
                            return 51;
                        case 9:
                            return 50;
                        case 10:
                            return 49;
                        case 11:
                            return 47;
                        case 12:
                            return 45;
                        case 13:
                            return 43;
                        case 14:
                            return 41;
                        case 15:
                            return 40;
                        case 16:
                            return 39;
                        case 17:
                            return 37;
                        case 18:
                            return 35;
                        case 19:
                            return 33;
                        case 20:
                            return 31;
                        case 21:
                            return 30;
                        case 22:
                            return 29;
                        case 23:
                            return 27;
                        case 24:
                            return 25;
                        case 25:
                            return 23;
                        case 26:
                            return 21;
                        case 27:
                            return 19;
                        case 28:
                            return 17;
                        case 29:
                            return 15;
                        case 30:
                            return 13;
                        case 31:
                            return 12;
                        case 32:
                            return 11;
                        case 33:
                            return 10;
                        case 34:
                            return 9;
                        case 35:
                            return 8;
                        case 36:
                            return 7;
                        case 37:
                            return 6;
                        case 38:
                            return 5;
                        case 39:
                            return 4;
                        case 40:
                            return 3;
                        case 41:
                            return 2;
                        case 42:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 42)
                        return 0;
                    else
                        log("SYSERR: Missing level for mage petrification saving throw.");
                    break;
                case SAVING_BREATH:    /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 73;
                        case 3:
                            return 71;
                        case 4:
                            return 69;
                        case 5:
                            return 67;
                        case 6:
                            return 65;
                        case 7:
                            return 63;
                        case 8:
                            return 61;
                        case 9:
                            return 60;
                        case 10:
                            return 59;
                        case 11:
                            return 57;
                        case 12:
                            return 55;
                        case 13:
                            return 53;
                        case 14:
                            return 51;
                        case 15:
                            return 50;
                        case 16:
                            return 49;
                        case 17:
                            return 47;
                        case 18:
                            return 45;
                        case 19:
                            return 43;
                        case 20:
                            return 41;
                        case 21:
                            return 40;
                        case 22:
                            return 39;
                        case 23:
                            return 37;
                        case 24:
                            return 35;
                        case 25:
                            return 33;
                        case 26:
                            return 31;
                        case 27:
                            return 29;
                        case 28:
                            return 27;
                        case 29:
                            return 25;
                        case 30:
                            return 23;
                        case 31:
                            return 22;
                        case 32:
                            return 21;
                        case 33:
                            return 20;
                        case 34:
                            return 19;
                        case 35:
                            return 18;
                        case 36:
                            return 17;
                        case 37:
                            return 16;
                        case 38:
                            return 15;
                        case 39:
                            return 14;
                        case 40:
                            return 13;
                        case 41:
                            return 12;
                        case 42:
                            return 11;
                        case 43:
                            return 10;
                        case 44:
                            return 9;
                        case 45:
                            return 8;
                        case 46:
                            return 7;
                        case 47:
                            return 6;
                        case 48:
                            return 4;
                        case 49:
                            return 2;
                        case 50:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 50)
                        return 0;
                    else
                        log("SYSERR: Missing level for mage breath weapon saving throw.");
                    break;
                case SAVING_SPELL:    /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 60;
                        case 2:
                            return 58;
                        case 3:
                            return 56;
                        case 4:
                            return 54;
                        case 5:
                            return 52;
                        case 6:
                            return 50;
                        case 7:
                            return 48;
                        case 8:
                            return 46;
                        case 9:
                            return 45;
                        case 10:
                            return 44;
                        case 11:
                            return 42;
                        case 12:
                            return 40;
                        case 13:
                            return 38;
                        case 14:
                            return 36;
                        case 15:
                            return 35;
                        case 16:
                            return 34;
                        case 17:
                            return 32;
                        case 18:
                            return 30;
                        case 19:
                            return 28;
                        case 20:
                            return 26;
                        case 21:
                            return 25;
                        case 22:
                            return 24;
                        case 23:
                            return 22;
                        case 24:
                            return 20;
                        case 25:
                            return 18;
                        case 26:
                            return 16;
                        case 27:
                            return 14;
                        case 28:
                            return 12;
                        case 29:
                            return 10;
                        case 30:
                            return 8;
                        case 31:
                            return 7;
                        case 32:
                            return 6;
                        case 33:
                            return 5;
                        case 34:
                            return 4;
                        case 35:
                            return 3;
                        case 36:
                            return 2;
                        case 37:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 37)
                        return 0;
                    else
                        log("SYSERR: Missing level for mage generic spells saving throw.");
                    break;
                default:
                    log("SYSERR: Invalid saving throw type.");
                    break;
            }
            break;
        case CLASS_CLERIC:
            switch (type) {
                case SAVING_PARA:    /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 60;
                        case 2:
                            return 59;
                        case 3:
                            return 48;
                        case 4:
                            return 46;
                        case 5:
                            return 45;
                        case 6:
                            return 43;
                        case 7:
                            return 40;
                        case 8:
                            return 37;
                        case 9:
                            return 35;
                        case 10:
                            return 34;
                        case 11:
                            return 33;
                        case 12:
                            return 31;
                        case 13:
                            return 30;
                        case 14:
                            return 29;
                        case 15:
                            return 27;
                        case 16:
                            return 26;
                        case 17:
                            return 25;
                        case 18:
                            return 24;
                        case 19:
                            return 23;
                        case 20:
                            return 22;
                        case 21:
                            return 21;
                        case 22:
                            return 20;
                        case 23:
                            return 18;
                        case 24:
                            return 15;
                        case 25:
                            return 14;
                        case 26:
                            return 12;
                        case 27:
                            return 10;
                        case 28:
                            return 9;
                        case 29:
                            return 8;
                        case 30:
                            return 7;
                        case 31:
                            return 6;
                        case 32:
                            return 5;
                        case 33:
                            return 4;
                        case 34:
                            return 3;
                        case 35:
                            return 2;
                        case 36:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 36)
                        return 0;
                    else
                        log("SYSERR: Missing level for cleric paralyzation saving throw.");
                    break;
                case SAVING_ROD:    /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 70;
                        case 2:
                            return 69;
                        case 3:
                            return 68;
                        case 4:
                            return 66;
                        case 5:
                            return 65;
                        case 6:
                            return 63;
                        case 7:
                            return 60;
                        case 8:
                            return 57;
                        case 9:
                            return 55;
                        case 10:
                            return 54;
                        case 11:
                            return 53;
                        case 12:
                            return 51;
                        case 13:
                            return 50;
                        case 14:
                            return 49;
                        case 15:
                            return 47;
                        case 16:
                            return 46;
                        case 17:
                            return 45;
                        case 18:
                            return 44;
                        case 19:
                            return 43;
                        case 20:
                            return 42;
                        case 21:
                            return 41;
                        case 22:
                            return 40;
                        case 23:
                            return 38;
                        case 24:
                            return 35;
                        case 25:
                            return 34;
                        case 26:
                            return 32;
                        case 27:
                            return 30;
                        case 28:
                            return 29;
                        case 29:
                            return 28;
                        case 30:
                            return 27;
                        case 31:
                            return 26;
                        case 32:
                            return 25;
                        case 33:
                            return 24;
                        case 34:
                            return 23;
                        case 35:
                            return 22;
                        case 36:
                            return 21;
                        case 37:
                            return 20;
                        case 38:
                            return 19;
                        case 39:
                            return 18;
                        case 40:
                            return 17;
                        case 41:
                            return 16;
                        case 42:
                            return 15;
                        case 43:
                            return 14;
                        case 44:
                            return 13;
                        case 45:
                            return 12;
                        case 46:
                            return 11;
                        case 47:
                            return 10;
                        case 48:
                            return 9;
                        case 49:
                            return 8;
                        case 50:
                            return 7;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 50)
                        return 0;
                    else
                        log("SYSERR: Missing level for cleric rod saving throw.");
                    break;
                case SAVING_PETRI:    /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 65;
                        case 2:
                            return 64;
                        case 3:
                            return 63;
                        case 4:
                            return 61;
                        case 5:
                            return 60;
                        case 6:
                            return 58;
                        case 7:
                            return 55;
                        case 8:
                            return 53;
                        case 9:
                            return 50;
                        case 10:
                            return 49;
                        case 11:
                            return 48;
                        case 12:
                            return 46;
                        case 13:
                            return 45;
                        case 14:
                            return 44;
                        case 15:
                            return 43;
                        case 16:
                            return 41;
                        case 17:
                            return 40;
                        case 18:
                            return 39;
                        case 19:
                            return 38;
                        case 20:
                            return 37;
                        case 21:
                            return 36;
                        case 22:
                            return 35;
                        case 23:
                            return 33;
                        case 24:
                            return 31;
                        case 25:
                            return 29;
                        case 26:
                            return 27;
                        case 27:
                            return 25;
                        case 28:
                            return 24;
                        case 29:
                            return 23;
                        case 30:
                            return 22;
                        case 31:
                            return 21;
                        case 32:
                            return 20;
                        case 33:
                            return 19;
                        case 34:
                            return 18;
                        case 35:
                            return 17;
                        case 36:
                            return 16;
                        case 37:
                            return 15;
                        case 38:
                            return 14;
                        case 39:
                            return 13;
                        case 40:
                            return 12;
                        case 41:
                            return 11;
                        case 42:
                            return 10;
                        case 43:
                            return 9;
                        case 44:
                            return 8;
                        case 45:
                            return 7;
                        case 46:
                            return 6;
                        case 47:
                            return 5;
                        case 48:
                            return 4;
                        case 49:
                            return 3;
                        case 50:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 50)
                        return 0;
                    else
                        log("SYSERR: Missing level for cleric petrification saving throw.");
                    break;
                case SAVING_BREATH:    /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 80;
                        case 2:
                            return 79;
                        case 3:
                            return 78;
                        case 4:
                            return 76;
                        case 5:
                            return 75;
                        case 6:
                            return 73;
                        case 7:
                            return 70;
                        case 8:
                            return 67;
                        case 9:
                            return 65;
                        case 10:
                            return 64;
                        case 11:
                            return 63;
                        case 12:
                            return 61;
                        case 13:
                            return 60;
                        case 14:
                            return 59;
                        case 15:
                            return 57;
                        case 16:
                            return 56;
                        case 17:
                            return 55;
                        case 18:
                            return 54;
                        case 19:
                            return 53;
                        case 20:
                            return 52;
                        case 21:
                            return 51;
                        case 22:
                            return 50;
                        case 23:
                            return 48;
                        case 24:
                            return 45;
                        case 25:
                            return 44;
                        case 26:
                            return 42;
                        case 27:
                            return 40;
                        case 28:
                            return 39;
                        case 29:
                            return 38;
                        case 30:
                            return 37;
                        case 31:
                            return 36;
                        case 32:
                            return 35;
                        case 33:
                            return 34;
                        case 34:
                            return 33;
                        case 35:
                            return 32;
                        case 36:
                            return 31;
                        case 37:
                            return 30;
                        case 38:
                            return 29;
                        case 39:
                            return 28;
                        case 40:
                            return 27;
                        case 41:
                            return 26;
                        case 42:
                            return 25;
                        case 43:
                            return 24;
                        case 44:
                            return 23;
                        case 45:
                            return 22;
                        case 46:
                            return 21;
                        case 47:
                            return 20;
                        case 48:
                            return 19;
                        case 49:
                            return 18;
                        case 50:
                            return 17;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 50)
                        return 0;
                    else
                        log("SYSERR: Missing level for cleric breath saving throw.");
                    break;
                case SAVING_SPELL:    /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 74;
                        case 3:
                            return 73;
                        case 4:
                            return 71;
                        case 5:
                            return 70;
                        case 6:
                            return 68;
                        case 7:
                            return 65;
                        case 8:
                            return 63;
                        case 9:
                            return 60;
                        case 10:
                            return 59;
                        case 11:
                            return 58;
                        case 12:
                            return 56;
                        case 13:
                            return 55;
                        case 14:
                            return 54;
                        case 15:
                            return 53;
                        case 16:
                            return 51;
                        case 17:
                            return 50;
                        case 18:
                            return 49;
                        case 19:
                            return 48;
                        case 20:
                            return 47;
                        case 21:
                            return 46;
                        case 22:
                            return 45;
                        case 23:
                            return 43;
                        case 24:
                            return 41;
                        case 25:
                            return 39;
                        case 26:
                            return 37;
                        case 27:
                            return 35;
                        case 28:
                            return 34;
                        case 29:
                            return 33;
                        case 30:
                            return 32;
                        case 31:
                            return 31;
                        case 32:
                            return 30;
                        case 33:
                            return 29;
                        case 34:
                            return 28;
                        case 35:
                            return 27;
                        case 36:
                            return 26;
                        case 37:
                            return 25;
                        case 38:
                            return 24;
                        case 39:
                            return 23;
                        case 40:
                            return 22;
                        case 41:
                            return 21;
                        case 42:
                            return 20;
                        case 43:
                            return 19;
                        case 44:
                            return 18;
                        case 45:
                            return 17;
                        case 46:
                            return 16;
                        case 47:
                            return 15;
                        case 48:
                            return 14;
                        case 49:
                            return 13;
                        case 50:
                            return 12;
                        default:
                            log("SYSERR: Missing level for cleric spell saving throw.");
                            break;
                    }
                    /* All remaining... */
                    if (level > 50)
                        return 0;
                    else
                        log("SYSERR: Missing level for cleric spell saving throw.");
                    break;
                default:
                    log("SYSERR: Invalid saving throw type.");
                    break;
            }
            break;
        case CLASS_RANGER:
        case CLASS_THIEF:
            switch (type) {
                case SAVING_PARA:    /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 65;
                        case 2:
                            return 64;
                        case 3:
                            return 63;
                        case 4:
                            return 62;
                        case 5:
                            return 61;
                        case 6:
                            return 60;
                        case 7:
                            return 59;
                        case 8:
                            return 58;
                        case 9:
                            return 57;
                        case 10:
                            return 56;
                        case 11:
                            return 55;
                        case 12:
                            return 54;
                        case 13:
                            return 53;
                        case 14:
                            return 52;
                        case 15:
                            return 51;
                        case 16:
                            return 50;
                        case 17:
                            return 49;
                        case 18:
                            return 48;
                        case 19:
                            return 47;
                        case 20:
                            return 46;
                        case 21:
                            return 45;
                        case 22:
                            return 44;
                        case 23:
                            return 43;
                        case 24:
                            return 42;
                        case 25:
                            return 41;
                        case 26:
                            return 40;
                        case 27:
                            return 39;
                        case 28:
                            return 38;
                        case 29:
                            return 37;
                        case 30:
                            return 36;
                        case 31:
                            return 35;
                        case 32:
                            return 34;
                        case 33:
                            return 33;
                        case 34:
                            return 32;
                        case 35:
                            return 31;
                        case 36:
                            return 30;
                        case 37:
                            return 29;
                        case 38:
                            return 28;
                        case 39:
                            return 27;
                        case 40:
                            return 26;
                        case 41:
                            return 25;
                        case 42:
                            return 24;
                        case 43:
                            return 23;
                        case 44:
                            return 22;
                        case 45:
                            return 21;
                        case 46:
                            return 20;
                        case 47:
                            return 19;
                        case 48:
                            return 18;
                        case 49:
                            return 17;
                        case 50:
                            return 16;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 50)
                        return 0;
                    else
                        log("SYSERR: Missing level for ranger / thief paralyzation saving throw.");
                    break;
                case SAVING_ROD:    /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 70;
                        case 2:
                            return 68;
                        case 3:
                            return 66;
                        case 4:
                            return 64;
                        case 5:
                            return 62;
                        case 6:
                            return 60;
                        case 7:
                            return 58;
                        case 8:
                            return 56;
                        case 9:
                            return 54;
                        case 10:
                            return 52;
                        case 11:
                            return 50;
                        case 12:
                            return 48;
                        case 13:
                            return 46;
                        case 14:
                            return 44;
                        case 15:
                            return 42;
                        case 16:
                            return 40;
                        case 17:
                            return 38;
                        case 18:
                            return 36;
                        case 19:
                            return 34;
                        case 20:
                            return 32;
                        case 21:
                            return 30;
                        case 22:
                            return 28;
                        case 23:
                            return 26;
                        case 24:
                            return 24;
                        case 25:
                            return 22;
                        case 26:
                            return 20;
                        case 27:
                            return 18;
                        case 28:
                            return 16;
                        case 29:
                            return 14;
                        case 30:
                            return 13;
                        case 31:
                            return 12;
                        case 32:
                            return 11;
                        case 33:
                            return 10;
                        case 34:
                            return 9;
                        case 35:
                            return 8;
                        case 36:
                            return 7;
                        case 37:
                            return 6;
                        case 38:
                            return 5;
                        case 39:
                            return 4;
                        case 40:
                            return 3;
                        case 41:
                            return 2;
                        case 42:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 42)
                        return 0;
                    else
                        log("SYSERR: Missing level for ranger / thief rod saving throw.");
                    break;

                case SAVING_PETRI:    /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 60;
                        case 2:
                            return 59;
                        case 3:
                            return 58;
                        case 4:
                            return 58;
                        case 5:
                            return 56;
                        case 6:
                            return 55;
                        case 7:
                            return 54;
                        case 8:
                            return 53;
                        case 9:
                            return 52;
                        case 10:
                            return 51;
                        case 11:
                            return 50;
                        case 12:
                            return 49;
                        case 13:
                            return 48;
                        case 14:
                            return 47;
                        case 15:
                            return 46;
                        case 16:
                            return 45;
                        case 17:
                            return 44;
                        case 18:
                            return 43;
                        case 19:
                            return 42;
                        case 20:
                            return 41;
                        case 21:
                            return 40;
                        case 22:
                            return 39;
                        case 23:
                            return 38;
                        case 24:
                            return 37;
                        case 25:
                            return 36;
                        case 26:
                            return 35;
                        case 27:
                            return 34;
                        case 28:
                            return 33;
                        case 29:
                            return 32;
                        case 30:
                            return 31;
                        case 31:
                            return 30;
                        case 32:
                            return 29;
                        case 33:
                            return 28;
                        case 34:
                            return 27;
                        case 35:
                            return 26;
                        case 36:
                            return 25;
                        case 37:
                            return 24;
                        case 38:
                            return 23;
                        case 39:
                            return 22;
                        case 40:
                            return 21;
                        case 41:
                            return 20;
                        case 42:
                            return 19;
                        case 43:
                            return 18;
                        case 44:
                            return 17;
                        case 45:
                            return 16;
                        case 46:
                            return 15;
                        case 47:
                            return 14;
                        case 48:
                            return 13;
                        case 49:
                            return 12;
                        case 50:
                            return 11;
                        default:
                            log("SYSERR: Missing level for thief petrification saving throw.");
                            break;
                    }
                    /* All remaining... */
                    if (level > 50)
                        return 0;
                    else
                        log("SYSERR: Missing level for ranger / thief petrification saving throw.");
                    break;

                case SAVING_BREATH:    /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 80;
                        case 2:
                            return 79;
                        case 3:
                            return 78;
                        case 4:
                            return 77;
                        case 5:
                            return 76;
                        case 6:
                            return 75;
                        case 7:
                            return 74;
                        case 8:
                            return 73;
                        case 9:
                            return 72;
                        case 10:
                            return 71;
                        case 11:
                            return 70;
                        case 12:
                            return 69;
                        case 13:
                            return 68;
                        case 14:
                            return 67;
                        case 15:
                            return 66;
                        case 16:
                            return 65;
                        case 17:
                            return 64;
                        case 18:
                            return 63;
                        case 19:
                            return 62;
                        case 20:
                            return 61;
                        case 21:
                            return 60;
                        case 22:
                            return 59;
                        case 23:
                            return 58;
                        case 24:
                            return 57;
                        case 25:
                            return 56;
                        case 26:
                            return 55;
                        case 27:
                            return 54;
                        case 28:
                            return 53;
                        case 29:
                            return 52;
                        case 30:
                            return 51;
                        case 31:
                            return 50;
                        case 32:
                            return 49;
                        case 33:
                            return 48;
                        case 34:
                            return 47;
                        case 35:
                            return 46;
                        case 36:
                            return 45;
                        case 37:
                            return 44;
                        case 38:
                            return 43;
                        case 39:
                            return 42;
                        case 40:
                            return 41;
                        case 41:
                            return 40;
                        case 42:
                            return 39;
                        case 43:
                            return 38;
                        case 44:
                            return 37;
                        case 45:
                            return 36;
                        case 46:
                            return 35;
                        case 47:
                            return 34;
                        case 48:
                            return 33;
                        case 49:
                            return 32;
                        case 50:
                            return 31;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 50)
                        return 0;
                    else
                        log("SYSERR: Missing level for ranger / thief breath saving throw.");
                    break;

                case SAVING_SPELL:    /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 73;
                        case 3:
                            return 71;
                        case 4:
                            return 69;
                        case 5:
                            return 67;
                        case 6:
                            return 65;
                        case 7:
                            return 63;
                        case 8:
                            return 61;
                        case 9:
                            return 59;
                        case 10:
                            return 57;
                        case 11:
                            return 55;
                        case 12:
                            return 53;
                        case 13:
                            return 51;
                        case 14:
                            return 49;
                        case 15:
                            return 47;
                        case 16:
                            return 45;
                        case 17:
                            return 43;
                        case 18:
                            return 41;
                        case 19:
                            return 39;
                        case 20:
                            return 37;
                        case 21:
                            return 35;
                        case 22:
                            return 33;
                        case 23:
                            return 31;
                        case 24:
                            return 29;
                        case 25:
                            return 27;
                        case 26:
                            return 25;
                        case 27:
                            return 23;
                        case 28:
                            return 21;
                        case 29:
                            return 19;
                        case 30:
                            return 17;
                        case 31:
                            return 16;
                        case 32:
                            return 15;
                        case 33:
                            return 14;
                        case 34:
                            return 13;
                        case 35:
                            return 12;
                        case 36:
                            return 11;
                        case 37:
                            return 10;
                        case 38:
                            return 9;
                        case 39:
                            return 8;
                        case 40:
                            return 7;
                        case 41:
                            return 6;
                        case 42:
                            return 5;
                        case 43:
                            return 4;
                        case 44:
                            return 3;
                        case 45:
                            return 2;
                        case 46:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 46)
                        return 0;
                    else
                        log("SYSERR: Missing level for ranger / thief spell saving throw.");
                    break;
                default:
                    log("SYSERR: Invalid saving throw type.");
                    break;
            }
            break;
        case CLASS_WARRIOR:
            switch (type) {
                case SAVING_PARA:    /* Paralyzation */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 70;
                        case 2:
                            return 68;
                        case 3:
                            return 67;
                        case 4:
                            return 65;
                        case 5:
                            return 62;
                        case 6:
                            return 58;
                        case 7:
                            return 55;
                        case 8:
                            return 53;
                        case 9:
                            return 52;
                        case 10:
                            return 50;
                        case 11:
                            return 47;
                        case 12:
                            return 43;
                        case 13:
                            return 40;
                        case 14:
                            return 38;
                        case 15:
                            return 37;
                        case 16:
                            return 35;
                        case 17:
                            return 32;
                        case 18:
                            return 28;
                        case 19:
                            return 25;
                        case 20:
                            return 24;
                        case 21:
                            return 23;
                        case 22:
                            return 22;
                        case 23:
                            return 20;
                        case 24:
                            return 19;
                        case 25:
                            return 17;
                        case 26:
                            return 16;
                        case 27:
                            return 15;
                        case 28:
                            return 14;
                        case 29:
                            return 13;
                        case 30:
                            return 12;
                        case 31:
                            return 11;
                        case 32:
                            return 10;
                        case 33:
                            return 9;
                        case 34:
                            return 8;
                        case 35:
                            return 7;
                        case 36:
                            return 6;
                        case 37:
                            return 5;
                        case 38:
                            return 4;
                        case 39:
                            return 3;
                        case 40:
                            return 2;
                        case 41:
                            return 1;    /* Some mobiles. */
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 41)
                        return 0;
                    else
                        log("SYSERR: Missing level for warrior paralyzation saving throw.");
                    break;
                case SAVING_ROD:    /* Rods */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 80;
                        case 2:
                            return 78;
                        case 3:
                            return 77;
                        case 4:
                            return 75;
                        case 5:
                            return 72;
                        case 6:
                            return 68;
                        case 7:
                            return 65;
                        case 8:
                            return 63;
                        case 9:
                            return 62;
                        case 10:
                            return 60;
                        case 11:
                            return 57;
                        case 12:
                            return 53;
                        case 13:
                            return 50;
                        case 14:
                            return 48;
                        case 15:
                            return 47;
                        case 16:
                            return 45;
                        case 17:
                            return 42;
                        case 18:
                            return 38;
                        case 19:
                            return 35;
                        case 20:
                            return 34;
                        case 21:
                            return 33;
                        case 22:
                            return 32;
                        case 23:
                            return 30;
                        case 24:
                            return 29;
                        case 25:
                            return 27;
                        case 26:
                            return 26;
                        case 27:
                            return 25;
                        case 28:
                            return 24;
                        case 29:
                            return 23;
                        case 30:
                            return 22;
                        case 31:
                            return 20;
                        case 32:
                            return 18;
                        case 33:
                            return 16;
                        case 34:
                            return 14;
                        case 35:
                            return 12;
                        case 36:
                            return 10;
                        case 37:
                            return 8;
                        case 38:
                            return 6;
                        case 39:
                            return 5;
                        case 40:
                            return 4;
                        case 41:
                            return 3;
                        case 42:
                            return 2;
                        case 43:
                            return 1;

                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 43)
                        return 0;
                    else
                        log("SYSERR: Missing level for warrior rod saving throw.");
                    break;

                case SAVING_PETRI:    /* Petrification */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 75;
                        case 2:
                            return 73;
                        case 3:
                            return 72;
                        case 4:
                            return 70;
                        case 5:
                            return 67;
                        case 6:
                            return 63;
                        case 7:
                            return 60;
                        case 8:
                            return 58;
                        case 9:
                            return 57;
                        case 10:
                            return 55;
                        case 11:
                            return 52;
                        case 12:
                            return 48;
                        case 13:
                            return 45;
                        case 14:
                            return 43;
                        case 15:
                            return 42;
                        case 16:
                            return 40;
                        case 17:
                            return 37;
                        case 18:
                            return 33;
                        case 19:
                            return 30;
                        case 20:
                            return 29;
                        case 21:
                            return 28;
                        case 22:
                            return 26;
                        case 23:
                            return 25;
                        case 24:
                            return 24;
                        case 25:
                            return 23;
                        case 26:
                            return 21;
                        case 27:
                            return 20;
                        case 28:
                            return 19;
                        case 29:
                            return 18;
                        case 30:
                            return 17;
                        case 31:
                            return 16;
                        case 32:
                            return 15;
                        case 33:
                            return 14;
                        case 34:
                            return 13;
                        case 35:
                            return 12;
                        case 36:
                            return 11;
                        case 37:
                            return 10;
                        case 38:
                            return 9;
                        case 39:
                            return 8;
                        case 40:
                            return 7;
                        case 41:
                            return 6;
                        case 42:
                            return 5;
                        case 43:
                            return 4;
                        case 44:
                            return 3;
                        case 45:
                            return 2;
                        case 46:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 46)
                        return 0;
                    else
                        log("SYSERR: Missing level for warrior petrification saving throw.");
                    break;
                case SAVING_BREATH:    /* Breath weapons */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 85;
                        case 2:
                            return 83;
                        case 3:
                            return 82;
                        case 4:
                            return 80;
                        case 5:
                            return 75;
                        case 6:
                            return 70;
                        case 7:
                            return 65;
                        case 8:
                            return 63;
                        case 9:
                            return 62;
                        case 10:
                            return 60;
                        case 11:
                            return 55;
                        case 12:
                            return 50;
                        case 13:
                            return 45;
                        case 14:
                            return 43;
                        case 15:
                            return 42;
                        case 16:
                            return 40;
                        case 17:
                            return 37;
                        case 18:
                            return 33;
                        case 19:
                            return 30;
                        case 20:
                            return 29;
                        case 21:
                            return 28;
                        case 22:
                            return 26;
                        case 23:
                            return 25;
                        case 24:
                            return 24;
                        case 25:
                            return 23;
                        case 26:
                            return 21;
                        case 27:
                            return 20;
                        case 28:
                            return 19;
                        case 29:
                            return 18;
                        case 30:
                            return 17;
                        case 31:
                            return 16;
                        case 32:
                            return 15;
                        case 33:
                            return 14;
                        case 34:
                            return 13;
                        case 35:
                            return 12;
                        case 36:
                            return 11;
                        case 37:
                            return 10;
                        case 38:
                            return 9;
                        case 39:
                            return 8;
                        case 40:
                            return 7;
                        case 41:
                            return 6;
                        case 42:
                            return 5;
                        case 43:
                            return 4;
                        case 44:
                            return 3;
                        case 45:
                            return 2;
                        case 46:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 46)
                        return 0;
                    else
                        log("SYSERR: Missing level for warrior breath saving throw.");
                    break;

                case SAVING_SPELL:    /* Generic spells */
                    switch (level) {
                        case 0:
                            return 90;
                        case 1:
                            return 85;
                        case 2:
                            return 83;
                        case 3:
                            return 82;
                        case 4:
                            return 80;
                        case 5:
                            return 77;
                        case 6:
                            return 73;
                        case 7:
                            return 70;
                        case 8:
                            return 68;
                        case 9:
                            return 67;
                        case 10:
                            return 65;
                        case 11:
                            return 62;
                        case 12:
                            return 58;
                        case 13:
                            return 55;
                        case 14:
                            return 53;
                        case 15:
                            return 52;
                        case 16:
                            return 50;
                        case 17:
                            return 47;
                        case 18:
                            return 43;
                        case 19:
                            return 40;
                        case 20:
                            return 39;
                        case 21:
                            return 38;
                        case 22:
                            return 36;
                        case 23:
                            return 35;
                        case 24:
                            return 34;
                        case 25:
                            return 33;
                        case 26:
                            return 31;
                        case 27:
                            return 30;
                        case 28:
                            return 29;
                        case 29:
                            return 28;
                        case 30:
                            return 27;
                        case 31:
                            return 25;
                        case 32:
                            return 23;
                        case 33:
                            return 21;
                        case 34:
                            return 19;
                        case 35:
                            return 17;
                        case 36:
                            return 15;
                        case 37:
                            return 13;
                        case 38:
                            return 11;
                        case 39:
                            return 9;
                        case 40:
                            return 7;
                        case 41:
                            return 6;
                        case 42:
                            return 5;
                        case 43:
                            return 4;
                        case 44:
                            return 3;
                        case 45:
                            return 2;
                        case 46:
                            return 1;
                        default:
                            break;
                    }
                    /* All remaining... */
                    if (level > 46)
                        return 0;
                    else
                        log("SYSERR: Missing level for warrior spell saving throw.");
                    break;

                default:
                    log("SYSERR: Invalid saving throw type.");
                    break;
            }
        default:
            log("SYSERR: Invalid class saving throw.");
            break;
    }
    /* Should not get here unless something is wrong. */
    return 100;
}

/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
int thaco(int class_num, int level) {
    switch (class_num) {
        // TODO: Turn all of these into a regression (no reason to have massive case statement)
        case CLASS_MAGIC_USER:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                case 2:
                case 3:
                    return 20;
                case 4:
                case 5:
                case 6:
                    return 19;
                case 7:
                case 8:
                case 9:
                    return 18;
                case 10:
                case 11:
                case 12:
                    return 17;
                case 13:
                case 14:
                case 15:
                    return 16;
                case 16:
                case 17:
                case 18:
                    return 15;
                case 19:
                case 20:
                case 21:
                    return 14;
                case 22:
                case 23:
                case 24:
                    return 13;
                case 25:
                case 26:
                case 27:
                    return 12;
                case 28:
                case 29:
                case 30:
                    return 11;
                case 31:
                case 32:
                case 33:
                    return 10;
                case 34:
                case 35:
                case 36:
                    return 9;
                case 37:
                case 38:
                case 39:
                    return 8;
                case 40:
                case 41:
                case 42:
                    return 7;
                case 43:
                case 44:
                case 45:
                    return 6;
                case 46:
                case 47:
                case 48:
                    return 5;
                case 49:
                case 50:
                    return 4;
            }
            // Deal with all remaining levels
            // Don't want to do this as default in case statement in case level
            // is somehow set to negative number.
            // TODO: if levels are added this needs to be fixed
            if (level > 50)
                return 1; /* IMMORTAL */
            else
                log("SYSERR: Missing level for magic user thac0.");

        case CLASS_CLERIC:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                case 2:
                case 3:
                    return 20;
                case 4:
                case 5:
                case 6:
                    return 18;
                case 7:
                case 8:
                case 9:
                    return 16;
                case 10:
                case 11:
                case 12:
                    return 14;
                case 13:
                case 14:
                case 15:
                    return 12;
                case 16:
                case 17:
                case 18:
                    return 10;
                case 19:
                case 20:
                case 21:
                    return 8;
                case 22:
                case 23:
                case 24:
                    return 6;
                case 25:
                case 26:
                case 27:
                    return 4;
                case 28:
                case 29:
                case 30:
                    return 2;
            }
            // Deal with all remaining levels
            // Don't want to do this as default in case statement in case level
            // is somehow set to negative number.
            if (level > 30)
                return 1;
            else
                log("SYSERR: Missing level for cleric thac0.");
        case CLASS_THIEF:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                case 2:
                    return 20;
                case 3:
                case 4:
                    return 19;
                case 5:
                case 6:
                    return 18;
                case 7:
                case 8:
                    return 17;
                case 9:
                case 10:
                    return 16;
                case 11:
                case 12:
                    return 15;
                case 13:
                case 14:
                    return 14;
                case 15:
                case 16:
                    return 13;
                case 17:
                case 18:
                    return 12;
                case 19:
                case 20:
                    return 11;
                case 21:
                case 22:
                    return 10;
                case 23:
                case 24:
                    return 9;
                case 25:
                case 26:
                    return 8;
                case 27:
                case 28:
                    return 7;
                case 29:
                case 30:
                    return 6;
                case 31:
                case 32:
                    return 5;
                case 33:
                case 34:
                    return 4;
                case 35:
                case 36:
                    return 3;
                case 37:
                case 38:
                    return 2;
            }
            // Deal with all remaining levels
            // Don't want to do this as default in case statement in case level
            // is somehow set to negative number.
            if (level > 38)
                return 1;
            else
                log("SYSERR: Missing level for thief thac0.");
        case CLASS_RANGER:
        case CLASS_WARRIOR:
            switch (level) {
                case 0:
                    return 100;
                case 1:
                    return 20;
                case 2:
                    return 19;
                case 3:
                    return 18;
                case 4:
                    return 17;
                case 5:
                    return 16;
                case 6:
                    return 15;
                case 7:
                    return 14;
                case 8:
                    return 14;
                case 9:
                    return 13;
                case 10:
                    return 12;
                case 11:
                    return 11;
                case 12:
                    return 10;
                case 13:
                    return 9;
                case 14:
                    return 8;
                case 15:
                    return 7;
                case 16:
                    return 6;
                case 17:
                    return 5;
                case 18:
                    return 4;
                case 19:
                    return 3;
                case 20:
                    return 2;
            }
            // Deal with all remaining levels
            // Don't want to do this as default in case statement in case level
            // is somehow set to negative number.
            if (level > 20)
                return 1;
            else
                log("SYSERR: Missing level for warrior / ranger thac0.");
        default:
            log("SYSERR: Unknown class in thac0 chart.");
    }

    /* Will not get there unless something is wrong. */
    return 100;
}


/*
 * Roll the 6 stats for a character... each stat is made of the sum of
 * the best 3 out of 4 rolls of a 6-sided die.  Each class then decides
 * which priority will be given for the best to worst stats.
 */
void roll_real_abils(struct char_data *ch) {
    int i, j, k, temp;
    ubyte table[6];
    ubyte rolls[4];

    for (i = 0; i < 6; i++)
        table[i] = 0;

    for (i = 0; i < 6; i++) {

        for (j = 0; j < 4; j++)
            rolls[j] = rand_number(1, 6);

        temp = rolls[0] + rolls[1] + rolls[2] + rolls[3] -
               MIN(rolls[0], MIN(rolls[1], MIN(rolls[2], rolls[3])));

        for (k = 0; k < 6; k++)
            if (table[k] < temp) {
                temp ^= table[k];
                table[k] ^= temp;
                temp ^= table[k];
            }
    }

    ch->real_abils.str_add = 0;

    switch (GET_CLASS(ch)) {
        case CLASS_MAGIC_USER:
            ch->real_abils.intel = table[0];
            ch->real_abils.wis = table[1];
            ch->real_abils.dex = table[2];
            ch->real_abils.str = table[3];
            ch->real_abils.con = table[4];
            ch->real_abils.cha = table[5];
            break;
        case CLASS_CLERIC:
            ch->real_abils.wis = table[0];
            ch->real_abils.intel = table[1];
            ch->real_abils.str = table[2];
            ch->real_abils.dex = table[3];
            ch->real_abils.con = table[4];
            ch->real_abils.cha = table[5];
            break;
        case CLASS_THIEF:
            ch->real_abils.dex = table[0];
            ch->real_abils.str = table[1];
            ch->real_abils.con = table[2];
            ch->real_abils.intel = table[3];
            ch->real_abils.wis = table[4];
            ch->real_abils.cha = table[5];
            break;
        case CLASS_WARRIOR:
            ch->real_abils.str = table[0];
            ch->real_abils.dex = table[1];
            ch->real_abils.con = table[2];
            ch->real_abils.wis = table[3];
            ch->real_abils.intel = table[4];
            ch->real_abils.cha = table[5];
            if (ch->real_abils.str == 18)
                ch->real_abils.str_add = rand_number(0, 100);
            break;
        case CLASS_RANGER:
            ch->real_abils.str = table[0];
            ch->real_abils.intel = table[1];
            ch->real_abils.dex = table[2];
            ch->real_abils.con = table[3];
            ch->real_abils.cha = table[4];
            ch->real_abils.wis = table[5];
            if (ch->real_abils.str == 18)
                ch->real_abils.str_add = rand_number(0, 100);
            break;
    }
    ch->aff_abils = ch->real_abils;
}


/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch) {
    GET_LEVEL(ch) = 1;
    GET_EXP(ch) = 1;

    set_title(ch, NULL);
    roll_real_abils(ch);

    GET_MAX_HIT(ch) = 10;
    GET_MAX_MANA(ch) = 100;
    GET_MAX_MOVE(ch) = 82;

    switch (GET_CLASS(ch)) {

        case CLASS_MAGIC_USER:
            break;

        case CLASS_CLERIC:
            break;

        case CLASS_THIEF:
            SET_SKILL(ch, SKILL_SNEAK, 10);
            SET_SKILL(ch, SKILL_HIDE, 5);
            SET_SKILL(ch, SKILL_STEAL, 15);
            SET_SKILL(ch, SKILL_BACKSTAB, 10);
            SET_SKILL(ch, SKILL_PICK_LOCK, 10);
            SET_SKILL(ch, SKILL_TRACK, 10);
            break;

        case CLASS_WARRIOR:
            break;

        case CLASS_RANGER:
            // TODO: Can set ranger skills here, but have to be implemented first
            break;
    }

    advance_level(ch);
    mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));

    GET_HIT(ch) = GET_MAX_HIT(ch);
    GET_MANA(ch) = GET_MAX_MANA(ch);
    GET_MOVE(ch) = GET_MAX_MOVE(ch);

    GET_COND(ch, THIRST) = 24;
    GET_COND(ch, FULL) = 24;
    GET_COND(ch, DRUNK) = 0;

    if (CONFIG_SITEOK_ALL)
        SET_BIT(PLR_FLAGS(ch), PLR_SITEOK);

    ch->player_specials->saved.olc_zone = NOWHERE;
}


/*
 * This function controls the change to maxmove, maxmana, and maxhp for
 * each class every time they gain a level.
 */
void advance_level(struct char_data *ch) {
    int add_hp, add_mana = 0, add_move = 0, i;

    add_hp = con_app[GET_CON(ch)].hitp;

    switch (GET_CLASS(ch)) {

        case CLASS_MAGIC_USER:
            add_hp += rand_number(3, 8);
            add_mana = rand_number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
            add_mana = MIN(add_mana, 10);
            add_move = rand_number(0, 2);
            break;

        case CLASS_CLERIC:
            add_hp += rand_number(5, 10);
            add_mana = rand_number(GET_LEVEL(ch), (int) (1.5 * GET_LEVEL(ch)));
            add_mana = MIN(add_mana, 10);
            add_move = rand_number(0, 2);
            break;

        case CLASS_THIEF:
            add_hp += rand_number(7, 13);
            add_mana = 0;
            add_move = rand_number(1, 3);
            break;

        case CLASS_WARRIOR:
            add_hp += rand_number(10, 15);
            add_mana = 0;
            add_move = rand_number(1, 3);
            break;

        case CLASS_RANGER:
            add_hp += rand_number(9, 13);
            add_mana = 0;
            add_move = rand_number(1, 3);
            break;
    }

    ch->points.max_hit += MAX(1, add_hp);
    ch->points.max_move += MAX(1, add_move);

    if (GET_LEVEL(ch) > 1)
        ch->points.max_mana += add_mana;

    if (IS_MAGIC_USER(ch) || IS_CLERIC(ch))
        GET_PRACTICES(ch) += MAX(2, wis_app[GET_WIS(ch)].bonus);
    else
        GET_PRACTICES(ch) += MIN(2, MAX(1, wis_app[GET_WIS(ch)].bonus));

    if (GET_LEVEL(ch) >= LVL_IMMORT) {
        for (i = 0; i < 3; i++)
            GET_COND(ch, i) = (char) -1;
        SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    }

    snoop_check(ch);
    save_char(ch);
}


/*
 * This simply calculates the backstab multiplier based on a character's
 * level.  This used to be an array, but was changed to be a function so
 * that it would be easier to add more levels to your MUD.  This doesn't
 * really create a big performance hit because it's not used very often.
 */
int backstab_mult(int level) {
    if (level <= 0)
        return 1;      /* level 0 */
    else if (level <= 7)
        return 2;      /* level 1 - 7 */
    else if (level <= 13)
        return 3;      /* level 8 - 13 */
    else if (level <= 20)
        return 4;      /* level 14 - 20 */
    else if (level <= 28)
        return 5;      /* level 21 - 28 */
    else if (level <= 35)
        return 6;      /* level 29 - 35 */
    else if (level <= 42)
        return 7;      /* levels 36 - 42 */
    else if (level < LVL_IMMORT)
        return 8;      /* all remaining mortal levels */
    else
        return 20;      /* immortals */
}


/*
 * invalid_class is used by handler.c to determine if a piece of equipment is
 * usable by a particular class, based on the ITEM_ANTI_{class} bitvectors.
 */
int invalid_class(struct char_data *ch, struct obj_data *obj) {
    if (OBJ_FLAGGED(obj, ITEM_ANTI_MAGIC_USER) && IS_MAGIC_USER(ch))
        return TRUE;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_CLERIC) && IS_CLERIC(ch))
        return TRUE;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_WARRIOR) && IS_WARRIOR(ch))
        return TRUE;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_THIEF) && IS_THIEF(ch))
        return TRUE;

    if (OBJ_FLAGGED(obj, ITEM_ANTI_RANGER) && IS_RANGER(ch))
        return TRUE;

    return FALSE;
}


/*
 * SPELLS AND SKILLS.  This area defines which spells are assigned to
 * which classes, and the minimum level the character must be to use
 * the spell or skill.
 */
void init_spell_levels(void) {
    /* MAGES */
    spell_level(SPELL_MAGIC_MISSILE, CLASS_MAGIC_USER, 1);
    spell_level(SPELL_DETECT_INVIS, CLASS_MAGIC_USER, 2);
    spell_level(SPELL_DETECT_MAGIC, CLASS_MAGIC_USER, 2);
    spell_level(SPELL_CHILL_TOUCH, CLASS_MAGIC_USER, 3);
    spell_level(SPELL_INFRAVISION, CLASS_MAGIC_USER, 3);
    spell_level(SPELL_INVISIBLE, CLASS_MAGIC_USER, 4);
    spell_level(SPELL_ARMOR, CLASS_MAGIC_USER, 4);
    spell_level(SPELL_BURNING_HANDS, CLASS_MAGIC_USER, 5);
    spell_level(SPELL_LOCATE_OBJECT, CLASS_MAGIC_USER, 6);
    spell_level(SPELL_STRENGTH, CLASS_MAGIC_USER, 6);
    spell_level(SPELL_SHOCKING_GRASP, CLASS_MAGIC_USER, 7);
    spell_level(SPELL_SLEEP, CLASS_MAGIC_USER, 8);
    spell_level(SPELL_LIGHTNING_BOLT, CLASS_MAGIC_USER, 9);
    spell_level(SPELL_BLINDNESS, CLASS_MAGIC_USER, 9);
    spell_level(SPELL_DETECT_POISON, CLASS_MAGIC_USER, 10);
    spell_level(SPELL_COLOR_SPRAY, CLASS_MAGIC_USER, 11);
    spell_level(SPELL_ENERGY_DRAIN, CLASS_MAGIC_USER, 13);
    spell_level(SPELL_CURSE, CLASS_MAGIC_USER, 14);
    spell_level(SPELL_POISON, CLASS_MAGIC_USER, 14);
    spell_level(SPELL_FIREBALL, CLASS_MAGIC_USER, 15);
    spell_level(SPELL_CHARM, CLASS_MAGIC_USER, 16);
    spell_level(SPELL_ENCHANT_WEAPON, CLASS_MAGIC_USER, 26);
    spell_level(SPELL_CLONE, CLASS_MAGIC_USER, 30);

    /* CLERICS */
    spell_level(SPELL_CURE_LIGHT, CLASS_CLERIC, 1);
    spell_level(SPELL_ARMOR, CLASS_CLERIC, 1);
    spell_level(SPELL_CREATE_FOOD, CLASS_CLERIC, 2);
    spell_level(SPELL_CREATE_WATER, CLASS_CLERIC, 2);
    spell_level(SPELL_DETECT_POISON, CLASS_CLERIC, 3);
    spell_level(SPELL_DETECT_ALIGN, CLASS_CLERIC, 4);
    spell_level(SPELL_CURE_BLIND, CLASS_CLERIC, 4);
    spell_level(SPELL_BLESS, CLASS_CLERIC, 5);
    spell_level(SPELL_DETECT_INVIS, CLASS_CLERIC, 6);
    spell_level(SPELL_BLINDNESS, CLASS_CLERIC, 6);
    spell_level(SPELL_INFRAVISION, CLASS_CLERIC, 7);
    spell_level(SPELL_PROT_FROM_EVIL, CLASS_CLERIC, 8);
    spell_level(SPELL_POISON, CLASS_CLERIC, 8);
    spell_level(SPELL_GROUP_ARMOR, CLASS_CLERIC, 9);
    spell_level(SPELL_CURE_CRITIC, CLASS_CLERIC, 9);
    spell_level(SPELL_SUMMON, CLASS_CLERIC, 10);
    spell_level(SPELL_REMOVE_POISON, CLASS_CLERIC, 10);
    spell_level(SPELL_WORD_OF_RECALL, CLASS_CLERIC, 12);
    spell_level(SPELL_EARTHQUAKE, CLASS_CLERIC, 12);
    spell_level(SPELL_DISPEL_EVIL, CLASS_CLERIC, 14);
    spell_level(SPELL_DISPEL_GOOD, CLASS_CLERIC, 14);
    spell_level(SPELL_SANCTUARY, CLASS_CLERIC, 15);
    spell_level(SPELL_CALL_LIGHTNING, CLASS_CLERIC, 15);
    spell_level(SPELL_HEAL, CLASS_CLERIC, 16);
    spell_level(SPELL_CONTROL_WEATHER, CLASS_CLERIC, 17);
    spell_level(SPELL_SENSE_LIFE, CLASS_CLERIC, 18);
    spell_level(SPELL_HARM, CLASS_CLERIC, 19);
    spell_level(SPELL_GROUP_HEAL, CLASS_CLERIC, 22);
    spell_level(SPELL_REMOVE_CURSE, CLASS_CLERIC, 26);

    /* THIEVES */
    spell_level(SKILL_SNEAK, CLASS_THIEF, 1);
    spell_level(SKILL_PICK_LOCK, CLASS_THIEF, 2);
    spell_level(SKILL_BACKSTAB, CLASS_THIEF, 3);
    spell_level(SKILL_STEAL, CLASS_THIEF, 4);
    spell_level(SKILL_HIDE, CLASS_THIEF, 5);
    spell_level(SKILL_TRACK, CLASS_THIEF, 6);

    /* WARRIORS */
    spell_level(SKILL_KICK, CLASS_WARRIOR, 1);
    spell_level(SKILL_RESCUE, CLASS_WARRIOR, 3);
    spell_level(SKILL_TRACK, CLASS_WARRIOR, 9);
    spell_level(SKILL_BASH, CLASS_WARRIOR, 12);

    //TODO: Need to add RANGER skills here
}


/*
 * This is the exp given to implementors -- it must always be greater
 * than the exp required for immortality, plus at least 20,000 or so.
 */
#define EXP_MAX  10000000

/* Function to return the exp required for each class/level */

// TODO: The XP table should probably be read in from a file
// TODO: Allow this to be done by regression formula, or by providing XP table on build
int level_exp(int chclass, int level) {
    if (level > LVL_IMPL || level < 0) {
        log("SYSERR: Requesting exp for invalid level %d!", level);
        return 0;
    }

    /*
   * Gods have exp close to EXP_MAX.  This statement should never have to
   * changed, regardless of how many mortal or immortal levels exist.
   */
    if (level > LVL_IMMORT) {
        return EXP_MAX - ((LVL_IMPL - level) * 1000);
    }

    /* Exp required for normal mortals is below */

    switch (chclass) {

        case CLASS_MAGIC_USER:
            // Special case for first 11 levels
            switch (level) {
                case 0:
                    return 0;
                case 1:
                    return 1;
                case 2:
                    return 2500;
                case 3:
                    return 5000;
                case 4:
                    return 10000;
                case 5:
                    return 20000;
                case 6:
                    return 40000;
                case 7:
                    return 60000;
                case 8:
                    return 90000;
                case 9:
                    return 135000;
                case 10:
                    return 250000;
                case 11:
                    return 375000;
            }
            /*
             * Add Formula to do this instead of large case
             * Magic user regression for XP
             */
            return (-338563 + (24780 * level) + (7929 * level * level));
            break;

        case CLASS_CLERIC:
            // Special case for first 11 levels
            switch (level) {
                case 0:
                    return 0;
                case 1:
                    return 1;
                case 2:
                    return 1500;
                case 3:
                    return 3000;
                case 4:
                    return 6000;
                case 5:
                    return 13000;
                case 6:
                    return 27500;
                case 7:
                    return 55000;
                case 8:
                    return 110000;
                case 9:
                    return 225000;
                case 10:
                    return 450000;
                case 11:
                    return 675000;
            }
            /*
             * Add Formula to do this instead of large case
             * Cleric regression for XP
             */
            return (-182695 + (6711 * level) + (7195 * level * level));
            break;

        case CLASS_THIEF:
            switch (level) {
                case 0:
                    return 0;
                case 1:
                    return 1;
                case 2:
                    return 1250;
                case 3:
                    return 2500;
                case 4:
                    return 5000;
                case 5:
                    return 10000;
                case 6:
                    return 20000;
                case 7:
                    return 40000;
                case 8:
                    return 70000;
                case 9:
                    return 110000;
                case 10:
                    return 160000;
                case 11:
                    return 220000;
            }
            /*
             * Add Formula to do this instead of large case
             * Thief regression for XP
             */
            return (-154970 + (-28827 * level) + (8924 * level * level));
            break;

        case CLASS_RANGER:
        case CLASS_WARRIOR:
            switch (level) {
                case 0:
                    return 0;
                case 1:
                    return 1;
                case 2:
                    return 2000;
                case 3:
                    return 4000;
                case 4:
                    return 8000;
                case 5:
                    return 16000;
                case 6:
                    return 32000;
                case 7:
                    return 64000;
                case 8:
                    return 125000;
                case 9:
                    return 250000;
                case 10:
                    return 500000;
                case 11:
                    return 750000;
            }
            /*
             * Add Formula to do this instead of large case
             * Warrior / Ranger regression for XP
             */
            return (-286785 + (24487 * level) + (7797 * level * level));
            break;
    }

    /*
   * This statement should never be reached if the exp tables in this function
   * are set up properly.  If you see exp of 123456 then the tables above are
   * incomplete -- so, complete them!
   */
    log("SYSERR: XP tables not set up correctly in class.c!");
    return 123456;
}


/*
 * Default titles of male characters.
 */
const char *title_male(int chclass, int level) {
    if (level <= 0 || level > LVL_IMPL)
        return "the Man";
    if (level == LVL_IMPL)
        return "the Implementor";

    switch (chclass) {

        case CLASS_MAGIC_USER:
            switch (level) {
                case 1:
                    return "the Apprentice of Magic";
                case 2:
                    return "the Spell Student";
                case 3:
                    return "the Scholar of Magic";
                case 4:
                    return "the Delver in Spells";
                case 5:
                    return "the Medium of Magic";
                case 6:
                    return "the Scribe of Magic";
                case 7:
                    return "the Seer";
                case 8:
                    return "the Sage";
                case 9:
                    return "the Illusionist";
                case 10:
                    return "the Abjurer";
                case 11:
                    return "the Invoker";
                case 12:
                    return "the Enchanter";
                case 13:
                    return "the Conjurer";
                case 14:
                    return "the Magician";
                case 15:
                    return "the Creator";
                case 16:
                    return "the Savant";
                case 17:
                    return "the Magus";
                case 18:
                    return "the Wizard";
                case 19:
                    return "the Warlock";
                case 20:
                    return "the Sorcerer";
                case 21:
                    return "the Necromancer";
                case 22:
                    return "the Thaumaturge";
                case 23:
                    return "the Student of the Occult";
                case 24:
                    return "the Disciple of the Uncanny";
                case 25:
                    return "the Minor Elemental";
                case 26:
                    return "the Greater Elemental";
                case 27:
                    return "the Crafter of Magics";
                case 28:
                    return "the Shaman";
                case 29:
                    return "the Keeper of Talismans";
                case 30:
                    return "the Archmage";
                case LVL_IMMORT:
                    return "the Immortal Warlock";
                case LVL_GOD:
                    return "the Avatar of Magic";
                case LVL_GRGOD:
                    return "the God of Magic";
                default:
                    return "the Mage";
            }
            break;

        case CLASS_CLERIC:
            switch (level) {
                case 1:
                    return "the Believer";
                case 2:
                    return "the Attendant";
                case 3:
                    return "the Acolyte";
                case 4:
                    return "the Novice";
                case 5:
                    return "the Missionary";
                case 6:
                    return "the Adept";
                case 7:
                    return "the Deacon";
                case 8:
                    return "the Vicar";
                case 9:
                    return "the Priest";
                case 10:
                    return "the Minister";
                case 11:
                    return "the Canon";
                case 12:
                    return "the Levite";
                case 13:
                    return "the Curate";
                case 14:
                    return "the Monk";
                case 15:
                    return "the Healer";
                case 16:
                    return "the Chaplain";
                case 17:
                    return "the Expositor";
                case 18:
                    return "the Bishop";
                case 19:
                    return "the Arch Bishop";
                case 20:
                    return "the Patriarch";
                    /* no one ever thought up these titles 21-30 */
                case LVL_IMMORT:
                    return "the Immortal Cardinal";
                case LVL_GOD:
                    return "the Inquisitor";
                case LVL_GRGOD:
                    return "the God of good and evil";
                default:
                    return "the Cleric";
            }
            break;

        case CLASS_THIEF:
            switch (level) {
                case 1:
                    return "the Pilferer";
                case 2:
                    return "the Footpad";
                case 3:
                    return "the Filcher";
                case 4:
                    return "the Pick-Pocket";
                case 5:
                    return "the Sneak";
                case 6:
                    return "the Pincher";
                case 7:
                    return "the Cut-Purse";
                case 8:
                    return "the Snatcher";
                case 9:
                    return "the Sharper";
                case 10:
                    return "the Rogue";
                case 11:
                    return "the Robber";
                case 12:
                    return "the Magsman";
                case 13:
                    return "the Highwayman";
                case 14:
                    return "the Burglar";
                case 15:
                    return "the Thief";
                case 16:
                    return "the Knifer";
                case 17:
                    return "the Quick-Blade";
                case 18:
                    return "the Killer";
                case 19:
                    return "the Brigand";
                case 20:
                    return "the Cut-Throat";
                    /* no one ever thought up these titles 21-30 */
                case LVL_IMMORT:
                    return "the Immortal Assasin";
                case LVL_GOD:
                    return "the Demi God of thieves";
                case LVL_GRGOD:
                    return "the God of thieves and tradesmen";
                default:
                    return "the Thief";
            }
            break;

        case CLASS_WARRIOR:
            switch (level) {
                case 1:
                    return "the Swordpupil";
                case 2:
                    return "the Recruit";
                case 3:
                    return "the Sentry";
                case 4:
                    return "the Fighter";
                case 5:
                    return "the Soldier";
                case 6:
                    return "the Warrior";
                case 7:
                    return "the Veteran";
                case 8:
                    return "the Swordsman";
                case 9:
                    return "the Fencer";
                case 10:
                    return "the Combatant";
                case 11:
                    return "the Hero";
                case 12:
                    return "the Myrmidon";
                case 13:
                    return "the Swashbuckler";
                case 14:
                    return "the Mercenary";
                case 15:
                    return "the Swordmaster";
                case 16:
                    return "the Lieutenant";
                case 17:
                    return "the Champion";
                case 18:
                    return "the Dragoon";
                case 19:
                    return "the Cavalier";
                case 20:
                    return "the Knight";
                    /* no one ever thought up these titles 21-30 */
                case LVL_IMMORT:
                    return "the Immortal Warlord";
                case LVL_GOD:
                    return "the Extirpator";
                case LVL_GRGOD:
                    return "the God of war";
                default:
                    return "the Warrior";
            }
            break;

        case CLASS_RANGER:
            switch (level) {
                case 1:
                    return "the Pathfinder";
                case 2:
                    return "the Outrider";
                case 3:
                    return "the Warder";
                case 4:
                    return "the Game Warden";
                case 5:
                    return "the Forest Walker";
                case 6:
                    return "the Forest Stalker";
                case 7:
                    return "the Hunter";
                case 8:
                    return "the Archer";
                case 9:
                    return "the Animal Trainer";
                case 10:
                    return "the Animal Handler";
                case 11:
                    return "the Wanderer";
                case 12:
                    return "the Swift Mover";
                case 13:
                    return "the Stealthy Forester";
                case 14:
                    return "the Swift Forester";
                case 15:
                    return "the Night Stalker";
                case 16:
                    return "the Huntsman";
                case 17:
                    return "the Earthgard";
                case 18:
                    return "the Windcaller";
                case 19:
                    return "the Farwarden";
                case 20:
                    return "the Plainswalker";
                    /* no one ever thought up these titles 21-30 */
                case LVL_IMMORT:
                    return "the Immortal Wanderer";
                case LVL_GOD:
                    return "the Stalker";
                case LVL_GRGOD:
                    return "the God of nature";
                default:
                    return "the Ranger";
            }
            break;
    }

    /* Default title for classes which do not have titles defined */
    return "the Classless";
}


/*
 * Default titles of female characters.
 */
const char *title_female(int chclass, int level) {
    if (level <= 0 || level > LVL_IMPL)
        return "the Woman";
    if (level == LVL_IMPL)
        return "the Implementress";

    switch (chclass) {

        case CLASS_MAGIC_USER:
            switch (level) {
                case 1:
                    return "the Apprentice of Magic";
                case 2:
                    return "the Spell Student";
                case 3:
                    return "the Scholar of Magic";
                case 4:
                    return "the Delveress in Spells";
                case 5:
                    return "the Medium of Magic";
                case 6:
                    return "the Scribess of Magic";
                case 7:
                    return "the Seeress";
                case 8:
                    return "the Sage";
                case 9:
                    return "the Illusionist";
                case 10:
                    return "the Abjuress";
                case 11:
                    return "the Invoker";
                case 12:
                    return "the Enchantress";
                case 13:
                    return "the Conjuress";
                case 14:
                    return "the Witch";
                case 15:
                    return "the Creator";
                case 16:
                    return "the Savant";
                case 17:
                    return "the Craftess";
                case 18:
                    return "the Wizard";
                case 19:
                    return "the War Witch";
                case 20:
                    return "the Sorceress";
                case 21:
                    return "the Necromancress";
                case 22:
                    return "the Thaumaturgess";
                case 23:
                    return "the Student of the Occult";
                case 24:
                    return "the Disciple of the Uncanny";
                case 25:
                    return "the Minor Elementress";
                case 26:
                    return "the Greater Elementress";
                case 27:
                    return "the Crafter of Magics";
                case 28:
                    return "Shaman";
                case 29:
                    return "the Keeper of Talismans";
                case 30:
                    return "Archwitch";
                case LVL_IMMORT:
                    return "the Immortal Enchantress";
                case LVL_GOD:
                    return "the Empress of Magic";
                case LVL_GRGOD:
                    return "the Goddess of Magic";
                default:
                    return "the Witch";
            }
            break;

        case CLASS_CLERIC:
            switch (level) {
                case 1:
                    return "the Believer";
                case 2:
                    return "the Attendant";
                case 3:
                    return "the Acolyte";
                case 4:
                    return "the Novice";
                case 5:
                    return "the Missionary";
                case 6:
                    return "the Adept";
                case 7:
                    return "the Deaconess";
                case 8:
                    return "the Vicaress";
                case 9:
                    return "the Priestess";
                case 10:
                    return "the Lady Minister";
                case 11:
                    return "the Canon";
                case 12:
                    return "the Levitess";
                case 13:
                    return "the Curess";
                case 14:
                    return "the Nunne";
                case 15:
                    return "the Healess";
                case 16:
                    return "the Chaplain";
                case 17:
                    return "the Expositress";
                case 18:
                    return "the Bishop";
                case 19:
                    return "the Arch Lady of the Church";
                case 20:
                    return "the Matriarch";
                    /* no one ever thought up these titles 21-30 */
                case LVL_IMMORT:
                    return "the Immortal Priestess";
                case LVL_GOD:
                    return "the Inquisitress";
                case LVL_GRGOD:
                    return "the Goddess of good and evil";
                default:
                    return "the Cleric";
            }
            break;

        case CLASS_THIEF:
            switch (level) {
                case 1:
                    return "the Pilferess";
                case 2:
                    return "the Footpad";
                case 3:
                    return "the Filcheress";
                case 4:
                    return "the Pick-Pocket";
                case 5:
                    return "the Sneak";
                case 6:
                    return "the Pincheress";
                case 7:
                    return "the Cut-Purse";
                case 8:
                    return "the Snatcheress";
                case 9:
                    return "the Sharpress";
                case 10:
                    return "the Rogue";
                case 11:
                    return "the Robber";
                case 12:
                    return "the Magswoman";
                case 13:
                    return "the Highwaywoman";
                case 14:
                    return "the Burglaress";
                case 15:
                    return "the Thief";
                case 16:
                    return "the Knifer";
                case 17:
                    return "the Quick-Blade";
                case 18:
                    return "the Murderess";
                case 19:
                    return "the Brigand";
                case 20:
                    return "the Cut-Throat";
                    /* no one ever thought up these titles 21-30 */
                case LVL_IMMORT:
                    return "the Immortal Assasin";
                case LVL_GOD:
                    return "the Demi Goddess of thieves";
                case LVL_GRGOD:
                    return "the Goddess of thieves and tradesmen";
                default:
                    return "the Thief";
            }
            break;

        case CLASS_WARRIOR:
            switch (level) {
                case 1:
                    return "the Swordpupil";
                case 2:
                    return "the Recruit";
                case 3:
                    return "the Sentress";
                case 4:
                    return "the Fighter";
                case 5:
                    return "the Soldier";
                case 6:
                    return "the Warrior";
                case 7:
                    return "the Veteran";
                case 8:
                    return "the Swordswoman";
                case 9:
                    return "the Fenceress";
                case 10:
                    return "the Combatess";
                case 11:
                    return "the Heroine";
                case 12:
                    return "the Myrmidon";
                case 13:
                    return "the Swashbuckleress";
                case 14:
                    return "the Mercenaress";
                case 15:
                    return "the Swordmistress";
                case 16:
                    return "the Lieutenant";
                case 17:
                    return "the Lady Champion";
                case 18:
                    return "the Lady Dragoon";
                case 19:
                    return "the Cavalier";
                case 20:
                    return "the Lady Knight";
                    /* no one ever thought up these titles 21-30 */
                case LVL_IMMORT:
                    return "the Immortal Lady of War";
                case LVL_GOD:
                    return "the Queen of Destruction";
                case LVL_GRGOD:
                    return "the Goddess of war";
                default:
                    return "the Warrior";
            }
            break;

        case CLASS_RANGER:
            switch (level) {
                case 1:
                    return "the Pathfinder";
                case 2:
                    return "the Outrider";
                case 3:
                    return "the Wardess";
                case 4:
                    return "the Game Wardess";
                case 5:
                    return "the Forest Walker";
                case 6:
                    return "the Forest Stalker";
                case 7:
                    return "the Huntress";
                case 8:
                    return "the Archeress";
                case 9:
                    return "the Animal Trainer";
                case 10:
                    return "the Animal Handler";
                case 11:
                    return "the Wanderess";
                case 12:
                    return "the Swift Mover";
                case 13:
                    return "the Stealthy Forester";
                case 14:
                    return "the Swift Forester";
                case 15:
                    return "the Night Stalker";
                case 16:
                    return "the Huntswoman";
                case 17:
                    return "the Earthgard";
                case 18:
                    return "the Windcaller";
                case 19:
                    return "the Farwardess";
                case 20:
                    return "the Plainswalker";
                    /* no one ever thought up these titles 21-30 */
                case LVL_IMMORT:
                    return "the Immortal Wanderer";
                case LVL_GOD:
                    return "the Stalker";
                case LVL_GRGOD:
                    return "the Goddess of nature";
                default:
                    return "the Ranger";
            }
            break;
    }

    /* Default title for classes which do not have titles defined */
    return "the Classless";
}

