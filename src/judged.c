/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * judged.c
 * Copyright (C) 2013 Sascha Reißner <reiszner@novaplan.at>
 * 
 * judged is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * judged is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>

#include "misc.h"
#include "config.h"
#include "ipc.h"
#include "incoming.h"
#include "master.h"

typedef struct wakeup_t {
	int mon;
	int day;
	int hrs;
	int min;
} Wakeup;

typedef struct pidlist_t {
	pid_t child;
	time_t clearance;
	struct pidlist_t *next;
} Pidlist;

typedef struct gameinfo_t {
	wchar_t name[10];
	time_t process;
	time_t deadline;
	time_t start;
	time_t grace;
	int    cpid;
	Pidlist *read;
	Pidlist *write;
	struct gameinfo_t *next;
} Gameinfo;

static int run = 1;
static int conf = 0;
static int childs = 0;
struct Config config, params;

/*
 *
 * signal-function
 *
 */

static void master_term (int signr) {
	logging(LOG_NOTICE, L"%s: SIGTERM received. prepare to quit...\n", config.judgecode);
	run = 0;
}

static void master_conf (int signr) {
	logging(LOG_NOTICE, L"%s: SIGHUP received. reread configfile...\n", config.judgecode);
	conf = 1;
}

/*
static void master_childs (int signr) {
	logging(LOG_NOTICE, L"%s: SIGCHLD received.\n", config.judgecode);
	childs++;
}
*/

/*
 *
 * daemon-function
 *
 */

static void start_daemon (const char *log_name, int facility) {
	int i;
	pid_t pid;

/* ignore SIGHUP */
	signal(SIGHUP, SIG_IGN);
	if ((pid = fork ()) != 0) exit (EXIT_SUCCESS);
	if (setsid() < 0) {
		logging(LOG_ERR, L"%s can't set sessionID. exit.\n", log_name);
		exit (EXIT_FAILURE);
	}
	chdir ("/");
	umask (0);

/* close all open filedescriptors and open log */
	for (i = sysconf (_SC_OPEN_MAX); i > 0; i--) close (i);
	openlog ( log_name, LOG_PID | LOG_CONS | LOG_NDELAY, facility );
}



int cleanup_input(pid_t *pid_childs) {
	int run = 0, i;
	for (i = 0 ; i < 7 ; i++) {
		if (pid_childs[i] > 0) {
			if (i==0)      logging(LOG_NOTICE, L"%s: sending 'SIGTERM' to fifo-child (%d)\n", config.judgecode, pid_childs[i]);
			else if (i==1) logging(LOG_NOTICE, L"%s: sending 'SIGTERM' to unix-child (%d)\n", config.judgecode, pid_childs[i]);
			else           logging(LOG_NOTICE, L"%s: sending 'SIGTERM' to inet-child (%d)\n", config.judgecode, pid_childs[i]);
			kill (pid_childs[i], SIGTERM);
			run++;
		}
	}
	return EXIT_SUCCESS;
}



int cleanup(int semid, int msgid, Gameinfo *all_games) {
	Gameinfo *game_temp = NULL;

	if (semid > 0) semctl(semid, 0, IPC_RMID, 0);
	if (msgid > 0) msgctl(msgid, IPC_RMID, 0);
	remove(config.pidfile);
	if (strlen(config.fifofile) > 0) remove(config.fifofile);
	if (strlen(config.unixsocket) > 0) remove(config.unixsocket);

	if (all_games != NULL) {
		game_temp = all_games;
		while (game_temp != NULL) {

/*
			logging(LOG_DEBUG, L"%s: Gamename    :'%s'\n", config.judgecode, game_temp->name);
			logging(LOG_DEBUG, L"%s: Gamestart   : %ld\n", config.judgecode, game_temp->start);
			logging(LOG_DEBUG, L"%s: Gameprocess : %ld\n", config.judgecode, game_temp->process);
			logging(LOG_DEBUG, L"%s: Gamedeadline: %ld\n", config.judgecode, game_temp->deadline);
			logging(LOG_DEBUG, L"%s: Gamegrace   : %ld\n", config.judgecode, game_temp->grace);
			logging(LOG_DEBUG, L"%s: ---------------------\n", config.judgecode);
*/

			all_games = game_temp->next;
			free (game_temp);
			game_temp = all_games;
		}
	}

	return EXIT_SUCCESS;
}



void config_out(struct Config *config, struct Config *params)
{
	logging(LOG_DEBUG, L"----- Config -----\n");
	logging(LOG_DEBUG, L"File:     %s\n", config->file);
	logging(LOG_DEBUG, L"User:     %s\n", config->judgeuser);
	logging(LOG_DEBUG, L"Group:    %s\n", config->judgegroup);
	logging(LOG_DEBUG, L"Dir:      %s\n", config->judgedir);
	logging(LOG_DEBUG, L"Code:     %s\n", config->judgecode);
	logging(LOG_DEBUG, L"Name: %s\n", config->judgename);
	logging(LOG_DEBUG, L"Address: %s\n", config->judgeaddr);
	logging(LOG_DEBUG, L"Keeper:   %s\n", config->judgekeeper);
	logging(LOG_DEBUG, L"Gateway:  %s\n", config->gateway);
	logging(LOG_DEBUG, L"Sendmail:  %s\n", config->sendmail);
	logging(LOG_DEBUG, L"PID-File: %s\n", config->pidfile);
	logging(LOG_DEBUG, L"UNIX:     %s\n", config->unixsocket);
	logging(LOG_DEBUG, L"INET:     %s\n", config->inetsocket);
	logging(LOG_DEBUG, L"Port:     %d\n", config->inetport);
	logging(LOG_DEBUG, L"FIFO:     %s\n", config->fifofile);
	logging(LOG_DEBUG, L"Childs:   %d\n", config->fifochilds);
	logging(LOG_DEBUG, L"Ilog:     %d\n", config->loginput);
	logging(LOG_DEBUG, L"Olog:     %d\n", config->logoutput);
	logging(LOG_DEBUG, L"Flag:     %d\n", config->restart);
	logging(LOG_DEBUG, L"----- Params -----\n");
	logging(LOG_DEBUG, L"File:     %s\n", params->file);
	logging(LOG_DEBUG, L"User:     %s\n", params->judgeuser);
	logging(LOG_DEBUG, L"Group:    %s\n", params->judgegroup);
	logging(LOG_DEBUG, L"Dir:      %s\n", params->judgedir);
	logging(LOG_DEBUG, L"Code:     %s\n", params->judgecode);
	logging(LOG_DEBUG, L"Name: %s\n", params->judgename);
	logging(LOG_DEBUG, L"Address: %s\n", params->judgeaddr);
	logging(LOG_DEBUG, L"Keeper:   %s\n", params->judgekeeper);
	logging(LOG_DEBUG, L"Gateway:  %s\n", params->gateway);
	logging(LOG_DEBUG, L"Sendmail:  %s\n", params->sendmail);
	logging(LOG_DEBUG, L"PID-File: %s\n", params->pidfile);
	logging(LOG_DEBUG, L"UNIX:     %s\n", params->unixsocket);
	logging(LOG_DEBUG, L"INET:     %s\n", params->inetsocket);
	logging(LOG_DEBUG, L"Port:     %d\n", params->inetport);
	logging(LOG_DEBUG, L"FIFO:     %s\n", params->fifofile);
	logging(LOG_DEBUG, L"Childs:   %d\n", params->fifochilds);
	logging(LOG_DEBUG, L"I-Log:    %d\n", params->loginput);
	logging(LOG_DEBUG, L"O-Log:    %d\n", params->logoutput);
	logging(LOG_DEBUG, L"Flag:     %d\n", params->restart);
}



void help() {
	wprintf(L"\nOptions:\n");
	wprintf(L"  -D         daemonize\n");
	wprintf(L"  -f <file>  configfile to load\n");
	wprintf(L"  -h         this helptext\n");
}



int check_file(const char *filename) {
	FILE *fp_file = NULL;

	umask (0137);
	if ((fp_file = fopen(filename, "r")) == NULL) {
		if ((fp_file = fopen(filename, "w")) == NULL) return -1;
		fclose (fp_file);
		if (chown(filename, config.judgeuid, config.judgegid) != 0) return -1;
	}
	else fclose (fp_file);
	return 0;
}

/*****************************************************************************/

int main(int argc, char **argv)
{

	struct message msg;
	struct tm *tmnow;
	Wakeup tmwake;

	int master_sort = 0, master_mark = 0, semid = 0, msgid = 0, res, msgpid, i, j;
	key_t ipc_key;
	FILE *fp_dip = NULL, *fp_master, *fp_temp;
	time_t now, wake;
	char gamename[1000][16], buffer[MSGLEN];
	wchar_t wbuffer[MSGLEN];
	pid_t pid, pid_childs[6];
	char temp[1024];

	Gameinfo *game_list = NULL, *game_temp = NULL; //, *game_del = NULL;
	Pidlist *pid_list = NULL, *pid_temp = NULL, *pid_del = NULL;



	if ( argc < 2 ) {
		fwprintf(stderr, L"%s: missing parameter\n",argv[0]);
		fwprintf(stderr, L"use '%s -h' for more information\n",argv[0]);
		return EXIT_FAILURE;
	}

	config.file[0] = '\0';
	config.judgeuser[0] = '\0';
	config.judgeuid = -1;
	config.judgegroup[0] = '\0';
	config.judgegid = -1;
	config.judgedir[0] = '\0';
	config.judgecode[0] = '\0';
	config.judgename[0] = '\0';
	config.judgeaddr[0] = '\0';
	config.judgekeeper[0] = '\0';
	config.gateway[0] = '\0';
	config.sendmail[0] = '\0';
	config.pidfile[0] = '\0';
	config.unixsocket[0] = '\0';
	config.inetsocket[0] = '\0';
	config.inetport = 0;
	config.fifofile[0] = '\0';
	config.fifochilds = 0;
	config.loginput = -1;
	config.logoutput = -1;
	config.restart = 0;

	memcpy(&params, &config, sizeof(struct Config));

	int Dflag = 0;
	int hflag = 0;
	int index, c;

	while ((c = getopt (argc, argv, "Df:h")) != -1)
		switch (c) {

			case 'D':
				Dflag = 1;
				break;

			case 'f':
				strcat(config.file, optarg);
				break;

			case 'h':
				hflag = 1;
				break;

			case '?':
				return EXIT_FAILURE;

			default:
				abort();

		}

	for (index = optind; index < argc; index++) {
		if (strlen(config.file) == 0)
			strcat(config.file, argv[index]);
		else
			fwprintf (stderr, L"Non-option argument %s\n", argv[index]);
	}

	if (hflag) {
		fwprintf(stdout, L"Usage: %s [-h] [-D] <[-f] configfile>\nstart a adjudicator for the boardgame 'Diplomacy'.\n\n", argv[0]);
		help();
		return EXIT_SUCCESS;
	}



/*
 *
 * we go now in background
 *
 */

	if (Dflag) {
		start_daemon("judged", LOG_DAEMON);
		if( setenv("JUDGE_DAEMON", "1", 1) != 0 ) {
			logging(LOG_ERR, L"%s: couldn't set JUDGE_DAEMON\n", config.judgecode);
			return EXIT_FAILURE;
		}
	}
	else {
		if( setenv("JUDGE_DAEMON", "0", 1) != 0 ) {
			logging(LOG_ERR, L"%s: couldn't set JUDGE_DAEMON\n", config.judgecode);
			return EXIT_FAILURE;
		}
	}

// read the configfile
//	config_out(&config, &params);

	for (i=0 ; i < 7 ; i++) pid_childs[i] = -1;
	if (read_config(&config, &params) == NULL) {
		logging(LOG_ERR, L"%s: error while read configfile '%s'. exit.\n", config.judgecode, config.file);
		return EXIT_FAILURE;
	}

//	config_out(&config, &params);



// check necessary files


	sprintf(temp, "%sdip.master", config.judgedir);
	if (check_file(temp)) {
		logging(LOG_ERR, L"%s: can't greate file '%s'. exit.\n", config.judgecode, temp);
		return EXIT_FAILURE;
	}
	else logging(LOG_DEBUG, L"%s: file '%s' exists.\n", config.judgecode, temp);

	sprintf(temp, "%sdip.whois", config.judgedir);
	if (check_file(temp)) {
		logging(LOG_ERR, L"%s: can't greate file '%s'. exit.\n", config.judgecode, temp);
		return EXIT_FAILURE;
	}
	else logging(LOG_DEBUG, L"%s: file '%s' exists.\n", config.judgecode, temp);

	sprintf(temp, "%sdip.ded", config.judgedir);
	if (check_file(temp)) {
		logging(LOG_ERR, L"%s: can't greate file '%s'. exit.\n", config.judgecode, temp);
		return EXIT_FAILURE;
	}
	else logging(LOG_DEBUG, L"%s: file '%s' exists.\n", config.judgecode, temp);




/*
 *
 * create PID-file
 *
 */

	logging(LOG_NOTICE, L"%s: create pidfile '%s' ...\n", config.judgecode, config.pidfile);
	umask (0133);
	if((fp_temp = fopen(config.pidfile,"r")) == NULL) {
		if((fp_temp = fopen(config.pidfile,"w")) == NULL) {
			logging(LOG_ERR, L"%s: can't create pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
			return EXIT_FAILURE;
		}
		else {
			fclose(fp_temp);
			if (chown(config.pidfile, config.judgeuid, config.judgegid) != 0) {
				logging(LOG_ERR, L"%s: can't change user and/or group of pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
				return EXIT_FAILURE;
			}
		}
	}
	else {
		logging(LOG_ERR, L"%s: existing pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
		return EXIT_FAILURE;
	}

	logging(LOG_NOTICE, L"%s: create IPC-Key ...\n", config.judgecode);
	ipc_key = ftok(config.file, 1);
	if(ipc_key == -1) {
		logging(LOG_ERR, L"%s: ftok failed with errno = %d\n", config.judgecode, errno);
		return EXIT_FAILURE;
	}

	semid = init_semaphore (ipc_key);
	if (semid < 0) {
		logging(LOG_ERR, L"%s: couldn't greate DIP-SemaphoreID.\n", config.judgecode);
		return EXIT_FAILURE;
	}
	if (semctl (semid, 0, SETVAL, (int) 1) == -1) {
		logging(LOG_ERR, L"%s: couldn't initialize DIP-SemaphoreID.\n", config.judgecode);
		return EXIT_FAILURE;
	}
	msgid = init_msgqueue (ipc_key);
	if (msgid < 0) {
		logging(LOG_ERR, L"%s: couldn't greate IPC-MessageID.\n", config.judgecode);
		return EXIT_FAILURE;
	}

	sprintf(temp,"%d",ipc_key);
	if( setenv("JUDGE_IPCKEY", temp, 1) != 0 ) {
		logging(LOG_ERR, L"%s: couldn't set JUDGE_IPCKEY\n", config.judgecode);
		return EXIT_FAILURE;
	}

	logging(LOG_NOTICE, L"%s: write to PID-file ...\n", config.judgecode);
	if((fp_temp = fopen(config.pidfile,"a")) == NULL) {
		logging(LOG_ERR, L"%s: can't write to pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
		return EXIT_FAILURE;
	}
	else {
		fprintf(fp_temp, "%d\n", getpid());
		fclose(fp_temp);
	}



// read in all games

	if((fp_master = fopen("dip.master","r")) == NULL) {
		logging(LOG_ERR, L"%s: An error occured while opening 'dip.master'.\n", config.judgecode);
		return EXIT_FAILURE;
	}

	master_mark = 0;
	while(fgetws(wbuffer, 4096, fp_master)) {
		if (master_mark == 0 && wcslen(wbuffer) > 2) {
			if (game_list == NULL) {
				if ((game_list = malloc(sizeof (*game_list))) == NULL) {
					logging(LOG_ERR, L"%s: error while malloc gameinfo.\n", config.judgecode);
					return EXIT_FAILURE;
				}
				game_temp = game_list;
			}
			else {
				if ((game_temp->next = malloc(sizeof (*game_list))) == NULL) {
					logging(LOG_ERR, L"%s: error while malloc gameinfo.\n", config.judgecode);
					return EXIT_FAILURE;
				}
				game_temp = game_temp->next;
			}
			game_temp->name[0] = L'\0';
			game_temp->process = 0;
			game_temp->start = 0;
			game_temp->grace = 0;
			game_temp->deadline = 0;
			game_temp->cpid = 0;
			game_temp->next = NULL;
			game_temp->read = NULL;
			game_temp->write = NULL;
		}

		if (wcsncmp(wbuffer,L"-",1) == 0) {
			master_mark = 0;
			continue;
		}

		if (master_mark == 0) {
			swscanf (wbuffer, L"%s", game_temp->name);
			master_mark = 1;
		}

		else {
			if (wcsncmp(wbuffer, L"S", 1) == 0)
				swscanf (wcspbrk(wbuffer, L"(") + 1, L"%ld", &game_temp->start);
			if (wcsncmp(wbuffer, L"P", 1) == 0)
				swscanf (wcspbrk(wbuffer, L"(") + 1, L"%ld", &game_temp->process);
			if (wcsncmp(wbuffer, L"D", 1) == 0)
				swscanf (wcspbrk(wbuffer, L"(") + 1, L"%ld", &game_temp->deadline);
			if (wcsncmp(wbuffer, L"G", 1) == 0)
				swscanf (wcspbrk(wbuffer, L"(") + 1, L"%ld", &game_temp->grace);
		}
	}
	fclose(fp_master);

/*
	if (all_games != NULL) {
		game_temp = all_games;
		while (game_temp != NULL) {
			logging(LOG_DEBUG, L"%s: Gamename    :'%s'\n", config.judgecode, game_temp->name);
			logging(LOG_DEBUG, L"%s: Gamestart   : %ld\n", config.judgecode, game_temp->start);
			logging(LOG_DEBUG, L"%s: Gameprocess : %ld\n", config.judgecode, game_temp->process);
			logging(LOG_DEBUG, L"%s: Gamedeadline: %ld\n", config.judgecode, game_temp->deadline);
			logging(LOG_DEBUG, L"%s: Gamegrace   : %ld\n", config.judgecode, game_temp->grace);
			logging(LOG_DEBUG, L"%s: ---------------------\n", config.judgecode);
			game_temp = game_temp->next;
		}
	}
*/

	wake = 0;
	tmwake.mon = tmwake.day = tmwake.hrs = tmwake.min = 0;
	signal (SIGTERM, master_term);
	signal (SIGINT, master_term);
	signal (SIGHUP, master_conf);
//	signal (SIGCHLD, master_childs);
	childs = 0;

	logging(LOG_NOTICE, L"%s: started.\n", config.judgecode);

	while(run > 0 || childs > 0 || pid_list != NULL) {
		usleep (FREQUENCY);

		if (conf > 0) {
			if (read_config(&config, &params) == NULL) {
				logging(LOG_ERR, L"%s: error while read configfile '%s'. exit.\n", config.judgecode, config.file);
				run = 0;
			}
			conf = 0;
		}

		if (run == 0) {
			cleanup_input(&pid_childs[0]);
			run = -1;
		}

/*
 *
 * check for restarts
 *
 */

		for (i=0 ; i < 7 ; i++) {
			if ( config.restart & (1<<i) ) {
				if (pid_childs[i] > 0) {
					if (i==0) {
						logging(LOG_NOTICE, L"%s: stoping fifo-child (%d) ...\n", config.judgecode, pid_childs[i]);
					}
					else if (i==1) {
						logging(LOG_NOTICE, L"%s: stoping unix-child (%d) ...\n", config.judgecode, pid_childs[i]);
					}
					else {
						logging(LOG_NOTICE, L"%s: stoping inet-child (%d) ...\n", config.judgecode, pid_childs[i]);
					}
					kill (pid_childs[i], SIGTERM);
				}
				else {
					pid_childs[i] = 0;
				}
				config.restart &= (127 - (1<<i));
			}
		}

/*
 * 
 * check if a child was ended
 * 
 */

		if (childs > 0) {
			res = waitpid (-1, NULL, WNOHANG);
			if (res < 0) {
				logging(LOG_ERR, L"%s: error while waitpid. errorcode: %d\n", config.judgecode, res);
//				childs--;
			}
			else if (res > 0) {
				for (i = 0 ; pid_childs[i] ; i++) {
					if (res == pid_childs[i]) {
						if (i==0) {
							logging(LOG_NOTICE, L"%s: fifo-child ended (%d)\n", config.judgecode, pid_childs[i]);
							remove(getenv("JUDGE_FIFO"));
							if (strlen(config.fifofile) > 0) {
								if( setenv("JUDGE_FIFO", config.fifofile, 1) != 0 ) {
									logging(LOG_ERR, L"%s: couldn't set JUDGE_FIFO\n", config.judgecode);
								}
								pid_childs[i] = 0;
							}
							else {
								pid_childs[i] = -1;
							}
						}
						else if (i==1) {
							logging(LOG_NOTICE, L"%s: unix-child ended (%d)\n", config.judgecode, pid_childs[i]);
							remove(getenv("JUDGE_UNIX"));
							if (strlen(config.unixsocket) > 0) {
								if( setenv("JUDGE_UNIX", config.unixsocket, 1) != 0 ) {
									logging(LOG_ERR, L"%s: couldn't set JUDGE_UNIX\n", config.judgecode);
								}
								pid_childs[i] = 0;
							}
							else {
								pid_childs[i] = -1;
							}
						}
						else {
							logging(LOG_NOTICE, L"%s: inet-child ended (%d)\n", config.judgecode, pid_childs[i]);
							sprintf(temp, "JUDGE_INET%d", i-2);
							if (getenv(temp)) {
								pid_childs[i] = 0;
							}
							else {
								pid_childs[i] = -1;
							}
						}
						childs--;
					}
				}

				if (pid_list != NULL) {
					pid_del = pid_temp = pid_list;
					while (pid_del->child != res) {
						pid_temp = pid_del;
						pid_del = pid_del->next;
					}
					if (pid_del == pid_list)
						pid_list = pid_list->next;
					else
						pid_temp->next = pid_del->next;
					logging(LOG_DEBUG, L"%s: process-child ended: %d (%p).\n", config.judgecode, pid_del->child, pid_del);
					free(pid_del);
					pid_del = NULL;
					pid_temp = NULL;
				}

				if (pid_list != NULL) {
					pid_temp = pid_list;
					while (pid_temp != NULL) {
						logging(LOG_DEBUG, L"%s: process-child: %d (%p).\n", config.judgecode, pid_temp->child, pid_temp);
						pid_temp = pid_temp->next;
					}
				}
				else
					logging(LOG_DEBUG, L"%s: no process-child.\n", config.judgecode);

			}
		}

/*
 * 
 * start childs
 * 
 */

		if (run > 0) {
			for (i=0 ; i < 7 ; i++) {
				if (pid_childs[i] == 0) {

/* FIFO */

					if (i==0) {
						sprintf(temp,"%d",config.fifochilds);
						if( setenv("JUDGE_FIFOCHILDS", temp, 1) != 0 ) {
							logging(LOG_ERR, L"%s: couldn't set JUDGE_FIFOCHILDS\n", config.judgecode);
							break;
						}
						if( setenv("JUDGE_FIFO", config.fifofile, 1) != 0 ) {
							logging(LOG_ERR, L"%s: couldn't set JUDGE_FIFO\n", config.judgecode);
							break;
						}
						if ((pid = fork()) < 0) {
							logging(LOG_ERR, L"%s: error while fork fifo-child.\n", config.judgecode);
						}

/* Parentprocess */

						else if (pid > 0) {
							pid_childs[i] = pid;
							childs++;
							logging(LOG_NOTICE, L"%s: fifo-child for '%s' forked with PID '%d'.\n", config.judgecode, config.fifofile, pid_childs[i]);
						}

/* Childprocess for FIFO */

						else {
#include "fifo.h"
							if (strlen(config.fifofile) > 0) {
								umask (0111);
								if (create_fifo(config.fifofile)) {
									logging(LOG_ERR, L"%s: couldn't create fifo '%s'.\n", config.judgecode, config.fifofile);
									return EXIT_FAILURE;
								}
							} else {
								logging(LOG_ERR, L"%s: no FIFO stated.\n", config.judgecode);
								return EXIT_FAILURE;
							}

							if (chown(config.fifofile, config.judgeuid, config.judgegid) != 0) {
								logging(LOG_ERR, L"%s: can't change user and/or group of fifo '%s'. exit.\n", config.judgecode, config.fifofile);
								return EXIT_FAILURE;
							}

							if ((res = chowngrp(config.judgeuid, config.judgegid)) != 0) {
								if (res == -1) logging(LOG_ERR, L"%s: can't change user. exit.\n", config.judgecode);
								if (res == -2) logging(LOG_ERR, L"%s: can't change group. exit.\n", config.judgecode);
								if (res < 0) return EXIT_FAILURE;
							}
							execlp("./judge-fifo", "judge-fifo", config.judgecode, config.fifofile, NULL);
						}
					}

/* UNIX */

					else if (i==1) {
						if( setenv("JUDGE_UNIX", config.unixsocket, 1) != 0 ) {
							logging(LOG_ERR, L"%s: couldn't set JUDGE_UNIX\n", config.judgecode);
							break;
						}
						if ((pid = fork ()) < 0) {
							logging(LOG_ERR, L"%s: error while fork unix-child.\n", config.judgecode);
						}

/* Parentprocess */

						else if (pid > 0) {
							pid_childs[i] = pid;
							childs++;
							logging(LOG_NOTICE, L"%s: unix-child for '%s' forked with PID '%d'.\n", config.judgecode, config.unixsocket, pid_childs[i]);
						}

/* Childprocess for UNIX */

						else {
							execlp("./judge-sock", "judge-sock", config.judgecode, config.unixsocket, NULL);
						}
					}

/* INET */

					else {
						sprintf(temp, "JUDGE_INET%d", i-2);
						if ((pid = fork ()) < 0) {
							logging(LOG_ERR, L"%s: error while fork inet-child for '%s'.\n", config.judgecode, getenv(temp));
						}

/* Parentprocess */

						else if (pid > 0) {
							pid_childs[i] = pid;
							childs++;
							logging(LOG_NOTICE, L"%s: inet-child for '%s' forked with PID '%d'.\n", config.judgecode, getenv(temp), pid_childs[i]);
						}

/* Childprocess for INET */

						else {
							if( setenv("JUDGE_INET", getenv(temp), 1) != 0 ) {
								logging(LOG_ERR, L"%s: couldn't set JUDGE_INET\n", config.judgecode);
								return EXIT_FAILURE;
							}
							execlp("./judge-sock", "judge-sock", config.judgecode, getenv(temp), NULL);
						}
					}
				}
			}
		}

/*
 * 
 * check messagequeue
 * 
 */

		res = msgrcv(msgid, &msg, MSGLEN, 1, IPC_NOWAIT);
		if (res < 0) {
			if (errno != ENOMSG) {
				logging(LOG_ERR, L"%s: error %d while reading messagequeue! res = %d\n", config.judgecode, errno, res);
//				return EXIT_FAILURE;
			}
		}
		else {
			logging(LOG_NOTICE, L"%s: incomming command '%ls' (%ld)\n", config.judgecode, msg.text, wcslen(msg.text));


// receive 'quit'
			if (wcsncmp(L"From quit", msg.text, 9) == 0) {
				run = 0;
				logging(LOG_NOTICE, L"%s: receive 'quit'\n", config.judgecode);
			}

// receive 'atrun'
			if (wcsncmp(L"From atrun ", msg.text, 11) == 0) {
				logging(LOG_NOTICE, L"%s: receive 'atrun'\n", config.judgecode);
				swscanf(msg.text + 11, L"%ld", &wake);
				tmnow = localtime(&wake);
				tmwake.mon = tmnow->tm_mon;
				tmwake.day = tmnow->tm_mday;
				tmwake.hrs = tmnow->tm_hour;
				tmwake.min = tmnow->tm_min;
				logging(LOG_NOTICE, L"%s: trigger set to '%d.%d. %02d:%02d (%ld)'.\n", config.judgecode, tmwake.day, tmwake.mon + 1, tmwake.hrs, tmwake.min, wake);
			}

// receive 'MESSAGE'
			if (wcsncmp(L"MESSAGE", msg.text, 7) == 0) {

				if (pid_list == NULL) {
					if ((pid_list = malloc(sizeof (*pid_list))) == NULL) {
						logging(LOG_ERR, L"%s: error while malloc pidlist.\n", config.judgecode);
						run = 0;
					}
					pid_temp = pid_list;
				}
				else {
					pid_temp = pid_list;
					while (pid_temp->next != NULL) {
						pid_temp = pid_temp->next;
					}
					if ((pid_temp->next = malloc(sizeof (*pid_list))) == NULL) {
						logging(LOG_ERR, L"%s: error while malloc member of pidlist.\n", config.judgecode);
						run = 0;
					}
					pid_temp = pid_temp->next;
				}
				pid_temp->child = 0;
				pid_temp->clearance = 0;
				pid_temp->next = NULL;

				if ((pid = fork ()) < 0) {
					logging(LOG_ERR, L"%s: error while fork child for process.\n", config.judgecode);
					run = 0;
				}
/* Parentprocess */
				else if (pid > 0) {
					logging(LOG_NOTICE, L"%s: judge-child forked with PID '%d'.\n", config.judgecode, pid);
					pid_temp->child = pid;
					pid_temp = NULL;
				}
/* Childprocess */
				else {
					swscanf(msg.text + 7, L"%d",&msgpid);
					if ((res = chowngrp(config.judgeuid, config.judgegid)) != 0) {
						if (res == -1) logging(LOG_ERR, L"%s: can't change user. exit.\n", config.judgecode);
						if (res == -2) logging(LOG_ERR, L"%s: can't change group. exit.\n", config.judgecode);
						if (res < 0) return EXIT_FAILURE;
					}
					incoming(msgpid);
					return EXIT_SUCCESS;
				}
			}

// receive 'READY'
			if (wcsncmp(L"READY", msg.text, 5) == 0) {
				swscanf(msg.text + 6, L"%d", &msgpid);
				game_temp = game_list;
				while (game_temp != NULL) {
					if (game_temp->cpid == msgpid) {
						logging(LOG_DEBUG, L"%s: send PROCESS '%s' to PID %d.\n", config.judgecode, game_temp->name, msgpid);
						send_msg(msgid, IPC_NOWAIT, msgpid, L"PROCESS %s\0", game_temp->name);
// *** debug ***
						game_temp->process = time(NULL) + 900;
						game_temp->cpid = 0;
						break;
					}
					game_temp = game_temp->next;
				}
			}
		}


/*
 * 
 * check if dip is free
 * 
 */

		if (semctl (semid, 0, GETVAL, 0) == UNLOCK) {
			time(&now);
			tmnow = localtime(&now);

// sort masterfile
			if (tmnow->tm_min != 59 && master_sort != 0)
				master_sort = 0;
			if (tmnow->tm_min == 59 && master_sort == 0) {
				semaphore_operation (semid, 0, LOCK);
				logging(LOG_NOTICE, L"%s: sort dip.master\n", config.judgecode);
				master_mark = 0;
				remove("dip.master.bak");
				rename("dip.master","dip.master.bak");
				if((fp_temp = fopen("dip.master.bak","r")) == NULL) {
					logging(LOG_ERR, L"%s: An error occured while opening 'dip.master.bak'.\n", config.judgecode);
					rename("dip.master.bak","dip.master");
				}
				else {
					while(fgets(buffer, MSGLEN, fp_temp)) {
						if (master_mark == 0 && strlen(buffer) > 2) {
							sscanf (buffer,"%s",gamename[master_sort]);
							master_mark = 1;
							master_sort++;
						}
						else {
							if (strncmp(buffer,"-",1) == 0) master_mark = 0;
						}
					}
					gamename[master_sort][0] = '\0';
					for (i = 0 ; i < master_sort ; i++) {
						for (j = i+1 ; j < master_sort ; j++) {
							if (strcmp(gamename[i],gamename[j]) > 0) {
								sscanf(gamename[j],"%s",gamename[master_sort]);
								sscanf(gamename[i],"%s",gamename[j]);
								sscanf(gamename[master_sort],"%s",gamename[i]);
								gamename[master_sort][0] = '\0';
							}
						}
					}
					if((fp_master = fopen("dip.master","w")) == NULL) {
						logging(LOG_ERR, L"%s: An error occured while opening 'dip.master'.\n", config.judgecode);
					}
					else {
						for (i = 0 ; i < master_sort ; i++) {
							rewind(fp_temp);
							master_mark = 0;
							while (master_mark < 2) {
								fgets(buffer, 4096, fp_temp);
								if (strncmp(buffer,gamename[i],strlen(gamename[i])) == 0) master_mark = 1;
								if (master_mark == 1) fputs(buffer, fp_master);
								if (strncmp(buffer,"-",1) == 0 && master_mark > 0) master_mark = 2;
							}
						}
					}
				}
				fclose(fp_master);
				fclose(fp_temp);
				master_sort = 1;
				semaphore_operation (semid, 0, UNLOCK);
			}

// force timer
			if (now > wake) {
				semaphore_operation (semid, 0, LOCK);
				sprintf(temp, "%sdip -x", config.judgedir);
				logging(LOG_NOTICE, L"%s: have no trigger, force one. '%s'\n", config.judgecode, temp);
				fp_dip = popen("true ; ./dip -x","w");
				if (fp_dip == NULL) {
					logging(LOG_ERR, L"%s: No conection to dip\n", config.judgecode);
				}
				else {
					pclose(fp_dip);
					fp_dip = NULL;
				}
				time(&wake);
				wake += 70;
				tmnow = localtime(&wake);
				tmwake.hrs = tmnow->tm_hour;
				tmwake.min = tmnow->tm_min + 1;
				tmwake.mon = tmnow->tm_mon;
				tmwake.day = tmnow->tm_mday;
				semaphore_operation (semid, 0, UNLOCK);
			}

// trigger timer
			if (game_list != NULL) {
				game_temp = game_list;
				while (game_temp != NULL) {
					if (game_temp->process <= now && game_temp->cpid == 0) {
						logging(LOG_INFO, L"%s: Process game '%s'...\n", config.judgecode, game_temp->name);
/* Fork */

						if (pid_list == NULL) {
							if ((pid_list = malloc(sizeof (*pid_list))) == NULL) {
								logging(LOG_ERR, L"%s: error while malloc pidlist.\n", config.judgecode);
								run = 0;
							}
							pid_temp = pid_list;
						}
						else {
							pid_temp = pid_list;
							while (pid_temp->next != NULL) {
								pid_temp = pid_temp->next;
							}
							if ((pid_temp->next = malloc(sizeof (*pid_list))) == NULL) {
								logging(LOG_ERR, L"%s: error while malloc member of pidlist.\n", config.judgecode);
								run = 0;
							}
							pid_temp = pid_temp->next;
						}
						pid_temp->child = 0;
						pid_temp->clearance = 0;
						pid_temp->next = NULL;

						if ((pid = fork ()) < 0) {
							logging(LOG_ERR, L"%s: error while fork judge-child.\n", config.judgecode);
							run = 0;
						}
/* Parentprocess */
						else if (pid > 0) {
							game_temp->cpid = pid;
							pid_temp->child = pid;
							pid_temp = NULL;
							logging(LOG_NOTICE, L"%s: child for processing forked with PID '%d'.\n", config.judgecode, pid);
						}
/* Childprocess */
						else {
							if ((res = chowngrp(config.judgeuid, config.judgegid)) != 0) {
								if (res == -1) logging(LOG_ERR, L"%s: can't change user. exit.\n", config.judgecode);
								if (res == -2) logging(LOG_ERR, L"%s: can't change group. exit.\n", config.judgecode);
								if (res < 0) return EXIT_FAILURE;
							}
							incoming(1);
							return EXIT_SUCCESS;
						}
/* Fork End */
						break;
					}
					game_temp = game_temp->next;
				}
			}

			if (tmnow->tm_hour == tmwake.hrs && tmnow->tm_min == tmwake.min && tmnow->tm_mon == tmwake.mon && tmnow->tm_mday == tmwake.day) {
				semaphore_operation (semid, 0, LOCK);
				sprintf(temp, "%sdip -x", config.judgedir);
				logging(LOG_NOTICE, L"%s: trigger '%s'\n", config.judgecode, temp);
				fp_dip = popen("true ; ./dip -x","w");
				if (fp_dip == NULL) {
					logging(LOG_ERR, L"%s: No conection to dip\n", config.judgecode);
				}
				else {
					pclose(fp_dip);
					fp_dip = NULL;
				}
				time(&wake);
				wake += 70;
				tmnow = localtime(&wake);
				tmwake.hrs = tmnow->tm_hour;
				tmwake.min = tmnow->tm_min + 1;
				tmwake.mon = tmnow->tm_mon;
				tmwake.day = tmnow->tm_mday;
				semaphore_operation (semid, 0, UNLOCK);
			}
		}

/*
		logging(LOG_DEBUG, L"%s: Run: %d\n", config.judgecode, run);
		logging(LOG_DEBUG, L"%s: childs: %d\n", config.judgecode, childs);
		if (pid_list)
			logging(LOG_DEBUG, L"%s: pid_list: %p\n", config.judgecode, pid_list);
		else
			logging(LOG_DEBUG, L"%s: pid_list: empty\n", config.judgecode);
*/

	}

	cleanup(semid, msgid, game_list);

	logging(LOG_NOTICE, L"%s: ended successfully.\n", config.judgecode);
	closelog();

	return EXIT_SUCCESS;
}
