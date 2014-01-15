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

struct wakeup {
	int mon;
	int day;
	int hrs;
	int min;
};

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
	char string_out[1024];
	sprintf(string_out, "%s: SIGTERM received. prepare to quit...\n", config.judgecode);
	output(LOG_NOTICE, string_out);
	run = 0;
}

static void master_conf (int signr) {
	char string_out[1024];
	sprintf(string_out, "%s: SIGHUP received. reread configfile...\n", config.judgecode);
	output(LOG_NOTICE, string_out);
	conf = 1;
}

static void master_childs (int signr) {
	char string_out[1024];
	sprintf(string_out, "%s: SIGCHLD received.\n", config.judgecode);
	output(LOG_NOTICE, string_out);
	childs++;
}

/*
 *
 * daemon-function
 *
 */

static void start_daemon (const char *log_name, int facility) {
	int i;
	pid_t pid;
	char string_out[1024];
/* ignore SIGHUP */
	signal(SIGHUP, SIG_IGN);
	if ((pid = fork ()) != 0) exit (EXIT_SUCCESS);
	if (setsid() < 0) {
		sprintf(string_out, "%s can't set sessionID. exit.\n", log_name);
		output(LOG_ERR, string_out);
		exit (EXIT_FAILURE);
	}
	chdir ("/");
	umask (0);
/* close all open filedescriptors and open log */
	for (i = sysconf (_SC_OPEN_MAX); i > 0; i--) close (i);
	openlog ( log_name, LOG_PID | LOG_CONS | LOG_NDELAY, facility );
}

int cleanup(int semid, int msgid, pid_t *pid_childs) {

// send all childs SIGTERM (15)

	char string_out[1024];
	int run = 0, res, i;
	for (i = 0 ; i < 7 ; i++) {
		if (pid_childs[i] > 0) {
			if (i==0) {
				sprintf(string_out, "%s: sending 'SIGTERM' to fifo-child (%d)\n", config.judgecode, pid_childs[i]);
				output(LOG_NOTICE, string_out);
			}
			else if (i==1) {
				sprintf(string_out, "%s: sending 'SIGTERM' to unix-child (%d)\n", config.judgecode, pid_childs[i]);
				output(LOG_NOTICE, string_out);
			}
			else {
				sprintf(string_out, "%s: sending 'SIGTERM' to inet-child (%d)\n", config.judgecode, pid_childs[i]);
				output(LOG_NOTICE, string_out);
			}
			kill (pid_childs[i], SIGTERM);
			run++;
		}
	}

	while (run > 0) {
		res = waitpid (-1, NULL, WNOHANG);
		if (res < 0) {
			sprintf(string_out, "%s: error while waitpid. errorcode: %d\n", config.judgecode, res);
			output(LOG_NOTICE, string_out);
		}
		else if (res > 0) {
			for (i = 0 ; i < 7 ; i++) {
				if (res == pid_childs[i]) {
					if (i==0) {
						sprintf(string_out, "%s: fifo-child ended (%d)\n", config.judgecode, pid_childs[i]);
						output(LOG_NOTICE, string_out);
					}
					else if (i==1) {
						sprintf(string_out, "%s: unix-child ended (%d)\n", config.judgecode, pid_childs[i]);
						output(LOG_NOTICE, string_out);
					}
					else {
						sprintf(string_out, "%s: inet-child ended (%d)\n", config.judgecode, pid_childs[i]);
						output(LOG_NOTICE, string_out);
					}
					pid_childs[i] = 0;
					run--;
				}
			}
		}
		else if (res == 0) usleep(100000);
	}

	if (semid > 0) semctl(semid, 0, IPC_RMID, 0);
	if (msgid > 0) msgctl(msgid, IPC_RMID, 0);
	remove(config.pidfile);
	if (strlen(config.fifofile) > 0) remove(config.fifofile);
	if (strlen(config.unixsocket) > 0) remove(config.unixsocket);
	sprintf( string_out, "%s: ended successfully.\n", config.judgecode);
	output(LOG_NOTICE, string_out);
	closelog();
	return EXIT_SUCCESS;
}

void config_out(struct Config *config, struct Config *params)
{
	char string_out[1024];

	output(LOG_NOTICE, "----- Config -----\n");
	sprintf(string_out, "File:     %s\n", config->file);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "User:     %s\n", config->judgeuser);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Group:    %s\n", config->judgegroup);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Dir:      %s\n", config->judgedir);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Code:     %s\n", config->judgecode);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Name: %s\n", config->judgename);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Address: %s\n", config->judgeaddr);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Keeper:   %s\n", config->judgekeeper);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Gateway:  %s\n", config->gateway);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Sendmail:  %s\n", config->sendmail);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "PID-File: %s\n", config->pidfile);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "UNIX:     %s\n", config->unixsocket);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "INET:     %s\n", config->inetsocket);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Port:     %d\n", config->inetport);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "FIFO:     %s\n", config->fifofile);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Childs:   %d\n", config->fifochilds);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Ilog:     %d\n", config->loginput);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Olog:     %d\n", config->logoutput);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Flag:     %d\n", config->restart);
	output(LOG_NOTICE, string_out);
	output(LOG_NOTICE, "----- Params -----\n");
	sprintf(string_out, "File:     %s\n", params->file);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "User:     %s\n", params->judgeuser);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Group:    %s\n", params->judgegroup);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Dir:      %s\n", params->judgedir);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Code:     %s\n", params->judgecode);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Name: %s\n", params->judgename);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Address: %s\n", params->judgeaddr);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Keeper:   %s\n", params->judgekeeper);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Gateway:  %s\n", params->gateway);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Sendmail:  %s\n", params->sendmail);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "PID-File: %s\n", params->pidfile);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "UNIX:     %s\n", params->unixsocket);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "INET:     %s\n", params->inetsocket);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Port:     %d\n", params->inetport);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "FIFO:     %s\n", params->fifofile);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Childs:   %d\n", params->fifochilds);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "I-Log:    %d\n", params->loginput);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "O-Log:    %d\n", params->logoutput);
	output(LOG_NOTICE, string_out);
	sprintf(string_out, "Flag:     %d\n", params->restart);
	output(LOG_NOTICE, string_out);
}

void help() {
	printf("\nOptions:\n");
	printf("  -D         daemonize\n");
	printf("  -f <file>  configfile to load\n");
	printf("  -h         this helptext\n");
}

/*****************************************************************************/

int main(int argc, char **argv)
{

	struct message msg;
	struct tm *tmnow;
	struct wakeup tmwake;

	int master_sort = 0, master_mark = 0, semid = 0, msgid = 0, res, i, j;
	key_t ipc_key;
	FILE *fp_dip = NULL, *fp_master, *fp_temp;
	time_t now, wake;
	char gamename[1000][16], buffer[MSGLEN];
	pid_t pid;
	char temp[1024], string_out[1024];
	pid_t pid_childs[6];



	if ( argc < 2 ) {
		fprintf(stderr, "%s: missing parameter\n",argv[0]);
		fprintf(stderr, "use '%s -h' for more information\n",argv[0]);
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
			fprintf (stderr, "Non-option argument %s\n", argv[index]);
	}

	if (hflag) {
		fprintf(stdout, "Usage: %s [-h] [-D] <[-f] configfile>\nmake a adjudicator for the boardgame 'Diplomacy'.\n\n",argv[0]);
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
			sprintf( string_out, "%s: couldn't set JUDGE_DAEMON\n", config.judgecode);
			output(LOG_ERR, string_out);
			return EXIT_FAILURE;
		}
	}
	else {
		if( setenv("JUDGE_DAEMON", "0", 1) != 0 ) {
			sprintf( string_out, "%s: couldn't set JUDGE_DAEMON\n", config.judgecode);
			output(LOG_ERR, string_out);
			return EXIT_FAILURE;
		}
	}

// read the configfile
	config_out(&config, &params);

	for (i=0 ; i < 7 ; i++) pid_childs[i] = -1;
	if (read_config(&config, &params) == NULL) {
		sprintf( string_out, "%s: error while read configfile '%s'. exit.\n", config.judgecode, config.file);
		output(LOG_NOTICE, string_out);
		return EXIT_FAILURE;
	}

	config_out(&config, &params);

/*
 *
 * create PID-file
 *
 */

	sprintf(string_out, "%s: create pidfile '%s' ...\n", config.judgecode, config.pidfile);
	output(LOG_NOTICE, string_out);
	umask (0133);
	if((fp_temp = fopen(config.pidfile,"r")) == NULL) {
		if((fp_temp = fopen(config.pidfile,"w")) == NULL) {
			sprintf( string_out, "%s: can't create pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
			output(LOG_ERR, string_out);
			return EXIT_FAILURE;
		}
		else {
			fclose(fp_temp);
			if (chown(config.pidfile, config.judgeuid, config.judgegid) != 0) {
				sprintf( string_out, "%s: can't change user and/or group of pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
				output(LOG_ERR, string_out);
				return EXIT_FAILURE;
			}
		}
	}
	else {
		sprintf( string_out, "%s: existing pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}

	sprintf( string_out, "%s: create IPC-Key ...\n", config.judgecode);
	output(LOG_NOTICE, string_out);
	ipc_key = ftok(config.file, 1);
	if(ipc_key == -1) {
		sprintf( string_out, "%s: ftok failed with errno = %d\n", config.judgecode, errno);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}

	semid = init_semaphore (ipc_key);
	if (semid < 0) {
		sprintf( string_out, "%s: couldn't greate DIP-SemaphoreID.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}
	if (semctl (semid, 0, SETVAL, (int) 1) == -1) {
		sprintf( string_out, "%s: couldn't initialize DIP-SemaphoreID.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}
	msgid = init_msgqueue (ipc_key);
	if (msgid < 0) {
		sprintf(string_out, "%s: couldn't greate IPC-MessageID.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}

	sprintf(temp,"%d",ipc_key);
	if( setenv("JUDGE_IPCKEY", temp, 1) != 0 ) {
		sprintf( string_out, "%s: couldn't set JUDGE_IPCKEY\n", config.judgecode);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}

	sprintf(string_out, "%s: write to PID-file ...\n", config.judgecode);
	output(LOG_NOTICE, string_out);
	if((fp_temp = fopen(config.pidfile,"a")) == NULL) {
		sprintf(string_out, "%s: can't write to pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}
	else {
		fprintf(fp_temp, "%d\n", getpid());
		fclose(fp_temp);
	}

	wake = 0;
	tmwake.mon = tmwake.day = tmwake.hrs = tmwake.min = 0;
	signal (SIGTERM, master_term);
	signal (SIGHUP, master_conf);
	signal (SIGCHLD, master_childs);
	childs = 0;
	sprintf(string_out, "%s: started.\n", config.judgecode);
	output(LOG_NOTICE, string_out);
	while(run) {
		usleep (1000000);

		if (conf > 0) {
			if (read_config(&config, &params) == NULL) {
				sprintf(string_out, "%s: error while read configfile '%s'. exit.\n", config.judgecode, config.file);
				output(LOG_ERR, string_out);
				run = 0;
			}
			conf = 0;
		}

/*
 *
 * check for restarts
 *
 */

		for (i=0 ; i < 7 ; i++) {
			if ( config.restart & (1<<i) ) {
				sprintf(string_out, "%s: loop-start = %d / restart %d ...\n", config.judgecode, i, config.restart);
				output(LOG_NOTICE, string_out);
				if (pid_childs[i] > 0) {
					if (i==0) {
						sprintf(string_out, "%s: stoping fifo-child (%d) ...\n", config.judgecode, pid_childs[i]);
						output(LOG_NOTICE, string_out);
					}
					else if (i==1) {
						sprintf(string_out, "%s: stoping unix-child (%d) ...\n", config.judgecode, pid_childs[i]);
						output(LOG_NOTICE, string_out);
					}
					else {
						sprintf(string_out, "%s: stoping inet-child (%d) ...\n", config.judgecode, pid_childs[i]);
						output(LOG_NOTICE, string_out);
					}
					kill (pid_childs[i], SIGTERM);
				}
				else {
					sprintf(string_out, "%s: check %d from %d ...\n", config.judgecode, i, pid_childs[i]);
					output(LOG_NOTICE, string_out);
					pid_childs[i] = 0;
/*
					if (i == 0) {
						if (strlen(config.fifofile) > 0) pid_childs[i] = 0;
						else pid_childs[i] = -1;
					}
					else if (i == 1) {
						if (strlen(config.unixsocket) > 0) pid_childs[i] = 0;
						else pid_childs[i] = -1;
					}
					else {
						sprintf(temp, "JUDGE_INET%d", i-2);
						if (getenv(temp) != NULL) pid_childs[i] = 0;
						else pid_childs[i] = -1;
					}
*/
					sprintf(string_out, "%s: set %d to %d ...\n", config.judgecode, i, pid_childs[i]);
					output(LOG_NOTICE, string_out);
				}
				config.restart &= (127 - (1<<i));
				sprintf(string_out, "%s: loop-end = %d / restart %d ...\n", config.judgecode, i, config.restart);
				output(LOG_NOTICE, string_out);
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
				sprintf(string_out, "%s: error while waitpid. errorcode: %d\n", config.judgecode, res);
				output(LOG_ERR, string_out);
				childs--;
			}
			else if (res > 0) {
				for (i = 0 ; pid_childs[i] ; i++) {
					if (res == pid_childs[i]) {
						if (i==0) {
							sprintf(string_out, "%s: fifo-child ended (%d)\n", config.judgecode, pid_childs[i]);
							output(LOG_NOTICE, string_out);
							remove(getenv("JUDGE_FIFO"));
							if (strlen(config.fifofile) > 0) {
								if( setenv("JUDGE_FIFO", config.fifofile, 1) != 0 ) {
									sprintf( string_out, "%s: couldn't set JUDGE_FIFO\n", config.judgecode);
									output(LOG_ERR, string_out);
								}
								pid_childs[i] = 0;
							}
							else {
//								unsetenv("JUDGE_FIFO");
								pid_childs[i] = -1;
							}
						}
						else if (i==1) {
							sprintf(string_out, "%s: unix-child ended (%d)\n", config.judgecode, pid_childs[i]);
							output(LOG_NOTICE, string_out);
							remove(getenv("JUDGE_UNIX"));
							if (strlen(config.unixsocket) > 0) {
								if( setenv("JUDGE_UNIX", config.unixsocket, 1) != 0 ) {
									sprintf( string_out, "%s: couldn't set JUDGE_UNIX\n", config.judgecode);
									output(LOG_ERR, string_out);
								}
								pid_childs[i] = 0;
							}
							else {
//								unsetenv("JUDGE_UNIX");
								pid_childs[i] = -1;
							}
						}
						else {
							sprintf(string_out, "%s: inet-child ended (%d)\n", config.judgecode, pid_childs[i]);
							output(LOG_NOTICE, string_out);
							sprintf(temp, "JUDGE_INET%d", i-2);
							if (getenv(temp)) {
								pid_childs[i] = 0;
							}
							else {
//								unsetenv(temp);
								pid_childs[i] = -1;
							}
						}
					}
				}
				childs--;
			}
		}

/*
 * 
 * start childs
 * 
 */

		for (i=0 ; i < 7 ; i++) {
			if (pid_childs[i] == 0) {
/* FIFO */
				if (i==0) {
					sprintf(temp,"%d",config.fifochilds);
					if( setenv("JUDGE_FIFOCHILDS", temp, 1) != 0 ) {
						sprintf( string_out, "%s: couldn't set JUDGE_FIFOCHILDS\n", config.judgecode);
						output(LOG_ERR, string_out);
						break;
					}
					if( setenv("JUDGE_FIFO", config.fifofile, 1) != 0 ) {
						sprintf( string_out, "%s: couldn't set JUDGE_FIFO\n", config.judgecode);
						output(LOG_ERR, string_out);
						break;
					}
					if ((pid = fork()) < 0) {
						sprintf(string_out, "%s: error while fork fifo-child.\n", config.judgecode);
						output(LOG_ERR, string_out);
					}
/* Parentprocess */
					else if (pid > 0) {
						pid_childs[i] = pid;
						sprintf(string_out, "%s: fifo-child for '%s' forked with PID '%d'.\n", config.judgecode, config.fifofile, pid_childs[i]);
						output(LOG_NOTICE, string_out);
					}
/* Childprocess for FIFO */
					else {
						execlp("./judge-fifo", "judge-fifo", config.judgecode, config.fifofile, NULL);
					}
				}
/* UNIX */
				else if (i==1) {
					if( setenv("JUDGE_UNIX", config.unixsocket, 1) != 0 ) {
						sprintf( string_out, "%s: couldn't set JUDGE_UNIX\n", config.judgecode);
						output(LOG_ERR, string_out);
						break;
					}
					if ((pid = fork ()) < 0) {
						sprintf(string_out, "%s: error while fork unix-child.\n", config.judgecode);
						output(LOG_ERR, string_out);
					}
/* Parentprocess */
					else if (pid > 0) {
						pid_childs[i] = pid;
						sprintf(string_out, "%s: unix-child for '%s' forked with PID '%d'.\n", config.judgecode, config.unixsocket, pid_childs[i]);
						output(LOG_NOTICE, string_out);
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
						sprintf(string_out, "%s: error while fork inet-child for '%s'.\n", config.judgecode, getenv(temp));
						output(LOG_ERR, string_out);
					}
/* Parentprocess */
					else if (pid > 0) {
						pid_childs[i] = pid;
						sprintf(string_out, "%s: inet-child for '%s' forked with PID '%d'.\n", config.judgecode, getenv(temp), pid_childs[i]);
						output(LOG_NOTICE, string_out);
					}
/* Childprocess for INET */
					else {
						if( setenv("JUDGE_INET", getenv(temp), 1) != 0 ) {
							sprintf( string_out, "%s: couldn't set JUDGE_INET\n", config.judgecode);
							output(LOG_ERR, string_out);
						}
						execlp("./judge-sock", "judge-sock", config.judgecode, getenv(temp), NULL);
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
				sprintf(string_out, "%s: error %d while reading messagequeue! res = %d\n", config.judgecode, errno, res);
				output(LOG_ERR, string_out);
//				return EXIT_FAILURE;
			}
		}
		else {
			sprintf(string_out, "%s: incomming command '%s' (%ld)\n", config.judgecode, msg.text, strlen(msg.text));
			output(LOG_NOTICE, string_out);

// receive 'quit'
			if (strncmp("From quit", msg.text, 9) == 0) {
				run = 0;
				sprintf(string_out, "%s: receive 'quit'\n", config.judgecode);
				output(LOG_NOTICE, string_out);
			}

// receive 'atrun'
			if (strncmp("From atrun ", msg.text, 11) == 0) {
				sprintf(string_out, "%s: receive 'atrun'\n", config.judgecode);
				output(LOG_NOTICE, string_out);
				sscanf(msg.text + 11,"%ld", &wake);
				tmnow = localtime(&wake);
				tmwake.mon = tmnow->tm_mon;
				tmwake.day = tmnow->tm_mday;
				tmwake.hrs = tmnow->tm_hour;
				tmwake.min = tmnow->tm_min;
				sprintf(string_out, "%s: trigger set to '%d.%d. %02d:%02d (%ld)'.\n", config.judgecode, tmwake.day, tmwake.mon + 1, tmwake.hrs, tmwake.min, wake);
				output(LOG_NOTICE, string_out);
			}

			if (strncmp("MESSAGE", msg.text, 7) == 0) {
				if ((pid = fork ()) < 0) {
					sprintf( string_out, "%s: error while fork judge-child.\n", config.judgecode);
					output(LOG_ERR, string_out);
					run = 0;
				}
/* Parentprocess */
				else if (pid > 0) {
					sprintf( string_out, "%s: judge-child forked with PID '%d'.\n", config.judgecode, pid);
					output(LOG_NOTICE, string_out);
				}
/* Childprocess */
				else {
					sscanf(msg.text + 7,"%d",&res);
					incoming(res);
					exit(0);
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
				sprintf( string_out, "%s: sort dip.master\n", config.judgecode);
				output(LOG_NOTICE, string_out);
				master_mark = 0;
				remove("dip.master.bak");
				rename("dip.master","dip.master.bak");
				if((fp_temp = fopen("dip.master.bak","r")) == NULL) {
					sprintf( string_out, "%s: An error occured while opening 'dip.master.bak'.\n", config.judgecode);
					output(LOG_ERR, string_out);
				}
				else {
					while(fgets(buffer, 4096, fp_temp)) {
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
						sprintf( string_out, "%s: An error occured while opening 'dip.master'.\n", config.judgecode);
						output(LOG_ERR, string_out);
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
				sprintf( string_out, "%s: have no trigger, force one. '%s'\n", config.judgecode, temp);
				output(LOG_NOTICE, string_out);
				fp_dip = popen("true ; ./dip -x","w");
				if (fp_dip == NULL) {
					sprintf( string_out, "%s: No conection to dip\n", config.judgecode);
					output(LOG_ERR, string_out);
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
			if (tmnow->tm_hour == tmwake.hrs && tmnow->tm_min == tmwake.min && tmnow->tm_mon == tmwake.mon && tmnow->tm_mday == tmwake.day) {
				semaphore_operation (semid, 0, LOCK);
				sprintf(temp, "%sdip -x", config.judgedir);
				sprintf( string_out, "%s: trigger '%s'\n", config.judgecode, temp);
				output(LOG_NOTICE, string_out);
				fp_dip = popen("true ; ./dip -x","w");
				if (fp_dip == NULL) {
					sprintf( string_out, "%s: No conection to dip\n", config.judgecode);
					output(LOG_ERR, string_out);
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
	}

	cleanup(semid, msgid, &pid_childs[0]);
	return EXIT_SUCCESS;
}
