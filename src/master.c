/***************************************************************************
 *            master.c
 *
 *  Sam Jänner 25 08:59:42 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "master.h"
#include "string_wcs.h"



long get_game_time(wchar_t *line) {
	long gametime;
	wchar_t *ptr;

	ptr = wcspbrk( line, L"(");
	gametime = wcstol( ptr+1, NULL, 10);

	return gametime;
}



void get_game_seq(wchar_t *line, Sequence *seq) {

	wchar_t *token, *ptr;

	wcs_stripe(line);
	token = wcstok(line, L" ", &ptr);
	while (token) {

		if (!wcsncmp(token, L"clock", 5)) {
			token = wcstok(NULL, L" ", &ptr);
			swscanf(token, L"%d", &seq->clock);
		}

		else if (!wcsncmp(token, L"min", 3)) {
			token = wcstok(NULL, L" ", &ptr);
			swscanf(token, L"%f", &seq->min);
		}

		else if (!wcsncmp(token, L"next", 4)) {
			token = wcstok(NULL, L" ", &ptr);
			swscanf(token, L"%f", &seq->next);
		}

		else if (!wcsncmp(token, L"grace", 5)) {
			token = wcstok(NULL, L" ", &ptr);
			swscanf(token, L"%f", &seq->grace);
		}

		else if (!wcsncmp(token, L"delay", 5)) {
			token = wcstok(NULL, L" ", &ptr);
			swscanf(token, L"%f", &seq->delay);
		}

		else if (!wcsncmp(token, L"days", 4)) {
			token = wcstok(NULL, L" ", &ptr);
			swscanf(token, L"%s", seq->delay);
		}

		token = wcstok(NULL, L" ", &ptr);
	}
	return;
}



Player *get_game_player(wchar_t *line) {

	Player *player;
	player = malloc(sizeof(Player));
	memset(player, 0, sizeof(Player));
	player->next = NULL;

	swscanf(line, L"%d %x %d %d %d %d %d %s %s",
	        &player->power, &player->flags, &player->units,
	        &player->centers, &player->userid, &player->siteid,
	        &player->warn, player->password, player->pref);

	return player;
}



int write_game(Game *game, FILE *master_fp) {

	fwprintf(master_fp, L"%s %s %s %d %d %d %d %x %d\n",
	         game->name, game->variant, game->phase,
	         &game->round, &game->access, &game->level,
	         &game->dedicate, &game->flags, &game->goal);

	if (game->process)   fwprintf(master_fp, L"Process   %24.24s (%ld)\n", ctime(&game->process),  game->process);
	if (game->deadline)  fwprintf(master_fp, L"Deadline  %24.24s (%ld)\n", ctime(&game->deadline), game->deadline);
	if (game->start)     fwprintf(master_fp, L"Start     %24.24s (%ld)\n", ctime(&game->start),    game->start);
	if (game->grace)     fwprintf(master_fp, L"Grace     %24.24s (%ld)\n", ctime(&game->grace),    game->grace);
	if (*game->comment)  fwprintf(master_fp, L"Comment   %s\n",game->comment);
	if (*game->epnum)    fwprintf(master_fp, L"EP_number %s\n",game->epnum);
	if (*game->bn_mnnum) fwprintf(master_fp, L"Number_BM %s\n",game->bn_mnnum);

	fwprintf(master_fp, L"Moves     clock %4d min %5.2f next %6.2f grace %6.2f delay %.2f days %s\n",
	         game->movement.clock, game->movement.min, game->movement.next,
	         game->movement.grace, game->movement.delay, game->movement.days);

	fwprintf(master_fp, L"Retreat   clock %4d min %5.2f next %6.2f grace %6.2f delay %.2f days %s\n",
	         game->retreat.clock, game->retreat.min, game->retreat.next,
	         game->retreat.grace, game->retreat.delay, game->retreat.days);

	fwprintf(master_fp, L"Adjust    clock %4d min %5.2f next %6.2f grace %6.2f delay %.2f days %s\n",
	         game->builds.clock, game->builds.min, game->builds.next,
	         game->builds.grace, game->builds.delay, game->builds.days);

	Player **next;
	next = &game->player;
	while (*next) {
		fwprintf(master_fp, L"%d %4x %2d %2d %3d %5d %3d %-12s %s\n",
		         (*next)->power, (*next)->flags, (*next)->units, (*next)->centers,
		         (*next)->userid, (*next)->siteid, (*next)->warn, (*next)->password, (*next)->pref);
		*next = (*next)->next;
	}

	fwprintf(master_fp, L"-\n");
	return 0;
}



int get_game_nums() {
	FILE *master_fp;
	char temp[1024], line[1024];
	int counter = 0;

	sprintf(temp, "%s/dip.master", getenv("JUDGE_DIR"));
	if((master_fp = fopen(temp, "r")) == NULL) return -1;

	while (fgets(line, 1024, master_fp)) {
		if (strncmp(line, "-", 1) == 0)
			counter++;
	}
	fclose(master_fp);
	return counter;
}



Game *get_game_by_num(int num) {
	FILE *master_fp;
	Game *game;
	Player **next;
	char temp[1024];
	wchar_t line[1024];
	int counter = 0;

	game = malloc(sizeof(Game));
	memset(game, 0, sizeof(Game));
	game->player = NULL;
	next = &game->player;

	sprintf(temp, "%s/dip.master", getenv("JUDGE_DIR"));
	if((master_fp = fopen(temp, "r")) == NULL) {
		free(game);
		game = NULL;
		return NULL;
	}

	while (fgetws(line, 1024, master_fp) && counter != num)
		if (wcsncmp(line, L"-", 1) == 0) counter++;

	while (fgetws(line, 1024, master_fp)) {
		if (wcsncmp(line, L"-", 1) == 0) break;
		else if (wcslen(game->name) == 0) {
			swscanf(line, L"%s %s %s %d %d %d %d %x %d",
			        game->name, game->variant, game->phase,
			        &game->round, &game->access, &game->level,
			        &game->dedicate, &game->flags, &game->goal);
		}
		else {
			switch (*line) {

				case 'P':
					game->process = get_game_time(line+9);
					break;

				case 'D':
					game->deadline = get_game_time(line+9);
					break;

				case 'S':
					game->start = get_game_time(line+9);
					break; 

				case 'G':
					game->grace = get_game_time(line+9);
					break; 

				case 'C':
					line[wcslen(line)-1] = 0L;
					wcsncpy(game->comment, line+9, sizeof(game->comment)-1);
					break;

				case 'M':
					get_game_seq(line, &game->movement);
					break;

				case 'R':
					get_game_seq(line, &game->retreat);
					break;

				case 'A':
				case 'B':
					get_game_seq(line, &game->builds);
					break;

				case 'E':
					line[wcslen(line)-1] = 0L;
					wcsncpy(game->epnum, line+9, sizeof(game->epnum)-1);
					break;

				case 'N':
					line[wcslen(line)-1] = 0L;
					wcsncpy(game->bn_mnnum, line+9, sizeof(game->bn_mnnum)-1);
					break;

				default:
					*next = get_game_player(line);
					next = &(*next)->next;
					game->subscribes++;

			}
		}
	}
	fclose(master_fp);
	if (wcslen(game->name) == 0) {
		free(game);
		game = NULL;
	}
	return game;
}



Game *get_game_by_name(wchar_t *name) {
	FILE *master_fp;
	Game *game;
	Player **next;
	char temp[1024];
	wchar_t line[1024];

	game = malloc(sizeof(Game));
	memset(game, 0, sizeof(Game));
	game->player = NULL;
	next = &game->player;

	sprintf(temp, "%s/dip.master", getenv("JUDGE_DIR"));
	if((master_fp = fopen(temp, "r")) == NULL) {
		free(game);
		game = NULL;
		return NULL;
	}

	while (fgetws(line, 1024, master_fp)) {
		if (!wcsncmp(line, name, wcslen(name))) {
			swscanf(line, L"%s %s %s %d %d %d %d %x %d",
			        game->name, game->variant, game->phase,
			        &game->round, &game->access, &game->level,
			        &game->dedicate, &game->flags, &game->goal);
			while(fgetws(line, 1024, master_fp) && wcsncmp(line, L"-", 1)) {
				switch (*line) {

					case 'P':
						game->process = get_game_time(line+9);
						break;

					case 'D':
						game->deadline = get_game_time(line+9);
						break;

					case 'S':
						game->start = get_game_time(line+9);
						break; 

					case 'G':
						game->grace = get_game_time(line+9);
						break; 

					case 'C':
						line[wcslen(line)-1] = 0L;
						wcsncpy(game->comment, line+9, sizeof(game->comment)-1);
						break;

					case 'M':
						get_game_seq(line, &game->movement);
						break;

					case 'R':
						get_game_seq(line, &game->retreat);
						break;

					case 'A':
					case 'B':
						get_game_seq(line, &game->builds);
						break;

					case 'E':
						line[wcslen(line)-1] = 0L;
						wcsncpy(game->epnum, line+9, sizeof(game->epnum)-1);
						break;

					case 'N':
						line[wcslen(line)-1] = 0L;
						wcsncpy(game->bn_mnnum, line+9, sizeof(game->bn_mnnum)-1);
						break;

					default:
						*next = get_game_player(line);
						next = &(*next)->next;
						game->subscribes++;
				}
			}
			break;
		}
		else while(fgetws(line, 1024, master_fp) && wcsncmp(line, L"-", 1));
	}
	fclose(master_fp);
	if (wcslen(game->name) == 0) {
		free(game);
		game = NULL;
	}
	return game;
}



int save_game(Game *game) {
	int res;
	FILE *newmaster_fp, *oldmaster_fp;
	char temp1[1024], temp2[1024], temp3[1024];
	wchar_t line[1024];

	sprintf(temp1, "%s/dip.master.new", getenv("JUDGE_DIR"));
	sprintf(temp2, "%s/dip.master",     getenv("JUDGE_DIR"));
	sprintf(temp3, "%s/dip.master.bak", getenv("JUDGE_DIR"));
	if((oldmaster_fp = fopen(temp2, "r")) == NULL) return -1;
	if((newmaster_fp = fopen(temp1, "w")) == NULL) {
		fclose(oldmaster_fp);
		return -2;
	}

	while (fgetws(line, 1024, oldmaster_fp)) {
		res = wcsncmp(line, game->name, wcslen(game->name));
		if (res < 1) {
			write_game(game, newmaster_fp);
		}
		else if (res > 1) {
			fputws(line, newmaster_fp);
			while(fgetws(line, 1024, oldmaster_fp) && wcsncmp(line, L"-", 1)) fputws(line, newmaster_fp);
			fputws(L"-\n", newmaster_fp);
		}
		else {
			while(fgetws(line, 1024, oldmaster_fp) && wcsncmp(line, L"-", 1));
			write_game(game, newmaster_fp);
		}
	}

	fclose(oldmaster_fp);
	fclose(newmaster_fp);
	remove(temp3);
	rename(temp2, temp3);
	rename(temp1, temp2);
	return 0;
}



int delete_game(wchar_t *name) {
	FILE *newmaster_fp, *oldmaster_fp;
	char temp1[1024], temp2[1024], temp3[1024];
	wchar_t line[1024];

	sprintf(temp1, "%s/dip.master.new", getenv("JUDGE_DIR"));
	sprintf(temp2, "%s/dip.master",     getenv("JUDGE_DIR"));
	sprintf(temp3, "%s/dip.master.bak", getenv("JUDGE_DIR"));
	if((oldmaster_fp = fopen(temp2, "r")) == NULL) return -1;
	if((newmaster_fp = fopen(temp1, "w")) == NULL) {
		fclose(oldmaster_fp);
		return -2;
	}

	while (fgetws(line, 1024, oldmaster_fp)) {
		if (wcsncmp(line, name, wcslen(name))) {
			fputws(line, newmaster_fp);
			while(fgetws(line, 1024, oldmaster_fp) && wcsncmp(line, L"-", 1)) fputws(line, newmaster_fp);
			fputws(L"-\n", newmaster_fp);
		}
	}

	fclose(oldmaster_fp);
	fclose(newmaster_fp);
	remove(temp3);
	rename(temp2, temp3);
	rename(temp1, temp2);
	return 0;
}
