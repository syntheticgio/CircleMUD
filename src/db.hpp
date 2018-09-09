
class Entity {
    virtual Entity();
    virtual ~Entity();
};

class Character : Entity {
    Character() {};
    ~Character() {};
};

class Mobile : Entity {
    Mobile() {};
    ~Mobile() {};
};


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