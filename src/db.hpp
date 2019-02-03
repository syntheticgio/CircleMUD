#include "structs.h"
#include <vector>
using namespace std;

class Entity
{

    virtual void myfunction() = 0;

    room_rnum in_room;     /* Location (real room number)	  */
    room_rnum was_in_room; /* location for linkdead people  */
    int wait;              /* wait for how many loops	  */

    struct char_player_data player;              /* Normal data                   */
    struct char_ability_data real_abils;         /* Abilities without modifiers   */
    struct char_ability_data aff_abils;          /* Abils with spells/stones/etc  */
    struct char_point_data points;               /* Points                        */
    struct char_special_data char_specials;      /* PC/NPC specials	  */
    struct player_special_data *player_specials; /* PC specials		  */

    vector<struct affected_type> affected_v;
    struct affected_type *affected; /* affected by what spells       */

    vector<struct obj_data> equipment_v;
    struct obj_data *equipment[NUM_WEARS]; /* Equipment array               */

    vector<struct obj_data> carrying_v;
    struct obj_data *carrying; /* Head of list                  */

    struct descriptor_data *desc; /* NULL for mobiles              */

    vector<struct char_data> next_in_room_v;
    struct char_data *next_in_room; /* For room->people - list         */
    vector<struct char_data> next_v;
    struct char_data *next; /* For either monster or ppl-list  */
    vector<struct char_data> next_fighting_v;
    struct char_data *next_fighting; /* For fighting list               */

    vector<struct follow_type> followers_v;
    struct follow_type *followers; /* List of chars followers       */
    vector<struct char_data> master_v;
    struct char_data *master; /* Who is char following?        */

    Entity() : equipment_v(NUM_WEARS){};
};

/**
 * Character base class, covers NPCs and characters
 */
class Character : Entity
{
    Character(){};
    ~Character(){};
};

/**
 * This would be the player class.
 */
class Player : Character
{
    Player(){};
    ~Player(){};

    int pfilepos; /* playerfile pos		  */
};

/**
 * This is the NPC class
 */
class NPC : Character
{
    NPC(){};
    ~NPC(){};
};

class Mobile : Entity
{
    Mobile(){};
    ~Mobile(){};

    mob_rnum nr;                          /* Mob's rnum			  */
    struct mob_special_data mob_specials; /* NPC specials		  */
};

/*
 *  If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
// struct player_special_data_saved {
//    byte skills[MAX_SKILLS+1];	/* array of skills plus skill 0		*/
//    byte PADDING0;		/* used to be spells_to_learn		*/
//    bool talks[MAX_TONGUE];	/* PC s Tongues 0 for NPC		*/
//    int	wimp_level;		/* Below this # of hit points, flee!	*/
//    byte freeze_level;		/* Level of god who froze char, if any	*/
//    sh_int invis_level;		/* level of invisibility		*/
//    room_vnum load_room;		/* Which room to place char in		*/
//    long /*bitvector_t*/	pref;	/* preference flags for PC's.		*/
//    ubyte bad_pws;		/* number of bad password attemps	*/
//    sbyte conditions[3];         /* Drunk, full, thirsty			*/

//    /* spares below for future expansion.  You can change the names from
//       'sparen' to something meaningful, but don't change the order.  */

//    ubyte spare0;
//    ubyte spare1;
//    ubyte spare2;
//    ubyte spare3;
//    ubyte spare4;
//    ubyte spare5;
//    int spells_to_learn;		/* How many can you learn yet this level*/
//    int olc_zone;
//    int spare8;
//    int spare9;
//    int spare10;
//    int spare11;
//    int spare12;
//    int spare13;
//    int spare14;
//    int spare15;
//    int spare16;
//    long	spare17;
//    long	spare18;
//    long	spare19;
//    long	spare20;
//    long	spare21;
// };

/* Special playing constants shared by PCs and NPCs which aren't in pfile */
// struct char_special_data {
//    struct char_data *fighting;	/* Opponent				*/
//    struct char_data *hunting;	/* Char hunted by this char		*/

//    byte position;		/* Standing, fighting, sleeping, etc.	*/

//    int	carry_weight;		/* Carried weight			*/
//    byte carry_items;		/* Number of items carried		*/
//    int	timer;			/* Timer for update			*/

//    struct char_special_data_saved saved; /* constants saved in plrfile	*/
// };

/*
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
// struct char_special_data_saved {
//    int	alignment;		/* +-1000 for alignments                */
//    long	idnum;			/* player's idnum; -1 for mobiles	*/
//    long /*bitvector_t*/ act;	/* act flag for NPC's; player flag for PC's */

//    long /*bitvector_t*/	affected_by;
// 				/* Bitvector for spells/skills affected by */
//    sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)		*/
// };

/* Char's points.  Used in char_file_u *DO*NOT*CHANGE* */
// struct char_point_data {
//    sh_int mana;
//    sh_int max_mana;     /* Max mana for PC/NPC			   */
//    sh_int hit;
//    sh_int max_hit;      /* Max hit for PC/NPC                      */
//    sh_int move;
//    sh_int max_move;     /* Max move for PC/NPC                     */

//    sh_int armor;        /* Internal -100..100, external -10..10 AC */
//    int	gold;           /* Money carried                           */
//    int	bank_gold;	/* Gold the char has in a bank account	   */
//    int	exp;            /* The experience of the player            */

//    sbyte hitroll;       /* Any bonus or penalty to the hit roll    */
//    sbyte damroll;       /* Any bonus or penalty to the damage roll */
// };

/* general player-related info, usually PC's and NPC's */
// struct char_player_data {
//    char	passwd[MAX_PWD_LENGTH+1]; /* character's password      */
//    char	*name;	       /* PC / NPC s name (kill ...  )         */
//    char	*short_descr;  /* for NPC 'actions'                    */
//    char	*long_descr;   /* for 'look'			       */
//    char	*description;  /* Extra descriptions                   */
//    char	*title;        /* PC / NPC's title                     */
//    byte sex;           /* PC / NPC's sex                       */
//    byte chclass;       /* PC / NPC's class		       */
//    byte level;         /* PC / NPC's level                     */
//    sh_int hometown;    /* PC s Hometown (zone)                 */
//    struct time_data time;  /* PC's AGE in days                 */
//    ubyte weight;       /* PC / NPC's weight                    */
//    ubyte height;       /* PC / NPC's height                    */
// };

// /* Char's abilities.  Used in char_file_u *DO*NOT*CHANGE* */
// struct char_ability_data {
//    sbyte str;
//    sbyte str_add;      /* 000 - 100 if strength 18             */
//    sbyte intel;
//    sbyte wis;
//    sbyte dex;
//    sbyte con;
//    sbyte cha;
// };

// /* ================== Structure for player/non-player ===================== */
// struct char_data {
//    int pfilepos;			 /* playerfile pos		  */
//    mob_rnum nr;                          /* Mob's rnum			  */
//    room_rnum in_room;                    /* Location (real room number)	  */
//    room_rnum was_in_room;		 /* location for linkdead people  */
//    int wait;				 /* wait for how many loops	  */

//    struct char_player_data player;       /* Normal data                   */
//    struct char_ability_data real_abils;	 /* Abilities without modifiers   */
//    struct char_ability_data aff_abils;	 /* Abils with spells/stones/etc  */
//    struct char_point_data points;        /* Points                        */
//    struct char_special_data char_specials;	/* PC/NPC specials	  */
//    struct player_special_data *player_specials; /* PC specials		  */
//    struct mob_special_data mob_specials;	/* NPC specials		  */

//    struct affected_type *affected;       /* affected by what spells       */
//    struct obj_data *equipment[NUM_WEARS];/* Equipment array               */

//    struct obj_data *carrying;            /* Head of list                  */
//    struct descriptor_data *desc;         /* NULL for mobiles              */

//    struct char_data *next_in_room;     /* For room->people - list         */
//    struct char_data *next;             /* For either monster or ppl-list  */
//    struct char_data *next_fighting;    /* For fighting list               */

//    struct follow_type *followers;        /* List of chars followers       */
//    struct char_data *master;             /* Who is char following?        */
// };
/* ====================================================================== */