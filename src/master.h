/***************************************************************************
 *            master.h
 *
 *  Sam Jänner 25 08:59:42 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

#ifndef _MASTER_

typedef struct sequence_t {
  int     clock;            /* Time of day for orders to be due             */
  int     min;              /* Minimum time before orders will be processed */
  int     next;             /* Maximum time before orders will be processed */
  int     grace;            /* Grace period for the tardy people            */
  int     delay;            /* Minimum delay after last orders received     */
  wchar_t days[8];          /* List of acceptable days, eg: xMTWTFx         */
} Sequence;

typedef struct player_t {
	int     power;          /* Power ordinal                              */
	int     flags;          /* Status flags                               */
	int     units;          /* Number of units this player has            */
	int     centers;        /* Number of centers this player has          */
	int     userid;         /* User's unique identifier                   */
	int     siteid;         /* User's site or area identifier             */
	int     warn;           /* Hours to warn before deadline              */
	wchar_t password[32];   /* Player's password                          */
	wchar_t address[256];   /* Player's electronic mail address           */
	wchar_t pref[256];      /* Player's power preference list             */
	struct player_t *next;  /* Pointer to the next player                 */
} Player;

typedef struct game_t {
	wchar_t name[9];        /* Game name                                    */
	wchar_t variant[32];    /* Variant: V_STANDARD                          */
	wchar_t phase[9];       /* Game phase of the form F1901M                */
	wchar_t comment[72];    /* Comment associated with game                 */
	wchar_t epnum[32];      /* Electronic Protocol number                   */
	wchar_t bn_mnnum[32];   /* Boardman/Miller number                       */
	int     round;          /* Game sequence number                         */
	int     access;         /* Access:  A_ANY, A_DIFF or A_SAME (site)      */
	int     level;          /* Level:   L_ANY, L_NOVICE, L_EXPERIENCED      */
	int     flags;          /* Flags:   F_NONMR                             */
	int     dedicate;       /* Minimum dedication requirement               */
	int     subscribes;     /* Number of players/observers in the game      */
	int     powers;         /* Number of powers for this variant            */
	int     goal;           /* Number of victory points required            */
	long    process;        /* Time to process this entry                   */
	long    start;          /* Minimum time before processing this entry    */
	long    deadline;       /* Current deadline to process this entry       */
	long    grace;          /* Absolute deadline including grace period     */
	Sequence movement;      /* Parameters for movement order deadlines      */
	Sequence retreat;       /* Parameters for retreat order deadlines       */
	Sequence builds;        /* Parameters for build order deadlines         */
	Player *player;         /* Pointer to the first player                  */
} Game;



int get_game_nums();
Game *get_game_by_num(int);
Game *get_game_by_name(wchar_t *);
int save_game(Game *);
int delete_game(wchar_t *);

#define _MASTER_
#endif