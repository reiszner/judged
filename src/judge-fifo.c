/***************************************************************************
 *            judge-fifo.c
 *
 *  Mon Juli 08 17:46:04 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024

#include "fifo.h"
#include "child.h"
#include "ipc.h"
#include "misc.h"

static int run = 1;
static void fifo_quit (int signr) {
	run = 0;
}

static void fifo_end (int signr) {
	exit(0);
}

int main(int argc, char **argv) {

	struct message msg;

	char judgedir[255], judgecode[255], gateway[255], fifofile[255];
	int judgeuid, judgegid, fifochilds = 0, childs = 0, res, i, j;
	pid_t pid;
	key_t ipc_key;

	sscanf(getenv("JUDGE_UID"), "%d", &judgeuid);
	sscanf(getenv("JUDGE_GID"), "%d", &judgegid);
	strcpy(judgedir, getenv("JUDGE_DIR"));
	strcpy(judgecode, getenv("JUDGE_CODE"));
	strcpy(gateway, getenv("JUDGE_GATEWAY"));
	strcpy(fifofile, getenv("JUDGE_FIFO"));
	sscanf(getenv("JUDGE_FIFOCHILDS"), "%d", &fifochilds);
	sscanf(getenv("JUDGE_IPCKEY"), "%d", &ipc_key);

	pid_t childs_pid[fifochilds];
	for (i = 0; i < fifochilds ; i++) childs_pid[i] = 0;

	openlog( argv[0], LOG_PID | LOG_CONS | LOG_NDELAY, LOG_DAEMON );

	if (strlen(fifofile) > 0) {
		umask (0111);
		if (create_fifo(fifofile)) {
			syslog(LOG_NOTICE, "%s: couldn't create fifo '%s'.\n", judgecode, fifofile);
			return EXIT_FAILURE;
		}
	} else {
		syslog(LOG_NOTICE, "%s: no FIFO stated.\n", judgecode);
		return EXIT_FAILURE;
	}

	if (chown(fifofile, judgeuid, judgegid) != 0) {
		syslog(LOG_NOTICE, "%s: can't change user and/or group of fifo '%s'. exit.\n", judgecode, fifofile);
		return EXIT_FAILURE;
	}

	if (getgid() == 0) {
		if (judgegid > 0) {
			if ((setgid(judgegid)) != 0) {
				syslog(LOG_NOTICE, "%s: problem while changing GID to %d\n", judgecode, judgegid);
				return EXIT_FAILURE;
			}
		}
		else {
			syslog(LOG_NOTICE, "%s: will not run with GID %d\n", judgecode, judgegid);
			return EXIT_FAILURE;
		}
	}

	if (getuid() == 0) {
		if (judgeuid > 0) {
			if ((setuid(judgeuid)) != 0) {
				syslog(LOG_NOTICE, "%s: problem while changing UID to %d\n", judgecode, judgeuid);
				return EXIT_FAILURE;
			}
		}
		else {
			syslog(LOG_NOTICE, "%s: will not run with UID %d\n", judgecode, judgeuid);
			return EXIT_FAILURE;
		}
	}

	signal (SIGQUIT, fifo_quit);
	while(run) {

		usleep (100000);

		if (childs > 0) {
			res = del_child(childs_pid, fifochilds);
			if (res < 0) syslog(LOG_NOTICE, "%s: error while waitpid. errorcode: %d\n", judgecode, res);
			else if (res > 0) {
				syslog(LOG_NOTICE, "%s: fifo-child '%d' ended.\n", judgecode, res);
				childs--;
			}
		}

// check if we have all childs running

		if (childs < fifochilds) {

			if ((pid = fork ()) < 0) {
				syslog(LOG_NOTICE, "%s: error while fork fifo-child.\n", judgecode);
				run = 0;
			}

/* Parentprocess */
			else if (pid > 0) {
				res = add_child(pid, childs_pid, fifochilds);
				if (res > 0) {
					syslog(LOG_NOTICE, "%s: fifo-child %d forked with PID '%d'.\n", judgecode, ++childs, res);
				}
			}

/* Childprocess */
			else {

				signal (SIGQUIT, fifo_end);

				if ((res = chowngrp(judgeuid, judgegid)) != 0) {
					if (res == -1) syslog(LOG_NOTICE, "%s: can't change user. exit.\n", judgecode);
					if (res == -2) syslog(LOG_NOTICE, "%s: can't change group. exit.\n", judgecode);
					if (res < 0) return EXIT_FAILURE;
				}

				int semid, msgid, cnt = 0, blocks = 0, pos = 0;
				FILE *fp_fifo, *fp_dipc;
				char *msgbuffer, *msg_begin, *msg_end, temp[1024];
				time_t now;

				semid = semget (ipc_key, 0, IPC_PRIVATE);
				if (semid < 0) return EXIT_FAILURE;
				msgid = msgget (ipc_key, IPC_PRIVATE);
				if (msgid < 0) return EXIT_FAILURE;

				semaphore_operation (semid, FIFO, LOCK);

				if((fp_fifo = fopen(fifofile, "r")) == NULL) {
					syslog( LOG_NOTICE, "%s: an error occured while opening the fifo '%s'.\n", judgecode, fifofile);
					semaphore_operation (semid, FIFO, UNLOCK);
					return EXIT_FAILURE;
				}

				msgbuffer = malloc(BUFFER_SIZE * sizeof(char));

				cnt = fread(msgbuffer, sizeof(char), BUFFER_SIZE, fp_fifo);
				signal (SIGQUIT, fifo_quit);
				pos += cnt;
				cnt = 0;

//				size_t fread(void *puffer, size_t blockgroesse, size_t blockzahl, FILE *datei);

				while (feof(fp_fifo) == 0) {
					if ( (int)(pos / BUFFER_SIZE) >= blocks) {
						syslog( LOG_NOTICE, "%s: pos = %d ; realloc to size %d (%d blocks).\n", judgecode, pos, BUFFER_SIZE * (blocks + 2), blocks + 2);
						msgbuffer = realloc(msgbuffer, sizeof(char) * BUFFER_SIZE * (++blocks + 1));
					}
					cnt = fread(&msgbuffer[pos * sizeof(char)], sizeof(char), BUFFER_SIZE, fp_fifo);
					pos += cnt;
					cnt = 0;
				}

				clearerr(fp_fifo);
				fclose(fp_fifo);
				semaphore_operation (semid, FIFO, UNLOCK);
				msgbuffer[pos] = '\0';

// Auswerten !!!
				semaphore_operation (semid, DIP, LOCK);

/*
				if (loginput > 0) {
					time(&now);
					sprintf(temp, "%s/log/%ld-%d.log", judgedir, now, getpid());
					syslog( LOG_NOTICE, "%s: write to logfile '%s'.\n", judgecode, temp);
					if((fp_temp = fopen(temp,"a")) == NULL) {
						syslog( LOG_NOTICE, "%s: can't create logfile '%s'.\n", judgecode, temp);
					}
					else {
						res = fwrite(msgbuffer, sizeof(char), strlen(msgbuffer), fp_temp);
						if ( res != strlen(msgbuffer))
							syslog( LOG_NOTICE, "%s: write to logfile has a problem - %d\n", judgecode, res);
						fclose(fp_temp);
					}
				}
*/

// Check if we have 2 Messages
				cnt = 0;
				msg_begin = msgbuffer;
				while (msg_begin != NULL) {
					*msg_begin = 'F';
					msg_end = strstr(msg_begin, "\nFrom ");
					if ( msg_end == NULL ) msg_end = msgbuffer;
					else *++msg_end = '\0';



// we have receive a QUIT
					if (strncmp("From quit\n", msg_begin, 10) == 0) {
						sprintf(msg.text, msg_begin);
						msg.prio=1;
						msgsnd(msgid, &msg, MSGLEN, 0);
					}



// we have receive a ATRUN
					else if (strncmp("From atrun ", msg_begin, 11) == 0) {
						sprintf(msg.text, msg_begin);
						msg.prio=1;
						msgsnd(msgid, &msg, MSGLEN, 0);
					}



// we have receive a message
					else {
						if ( strstr(msg_begin, gateway) == NULL) {
							strncpy(temp, msg_begin, 1024);
							temp[1023] = '\0';
							syslog( LOG_NOTICE, "%s: incoming message '%s' (size: %ld)\n", judgecode, strtok(temp, "\n"), (long int) strlen(msg_begin));
						}

/*
						if (loginput > 0) {
							time(&now);
							sprintf(temp, "%s/log/%ld-%d-%d.log", judgedir, now, getpid(), cnt++);
							syslog( LOG_NOTICE, "%s: write to logfile '%s'.\n", judgecode, temp);
							if((fp_temp = fopen(temp,"a")) == NULL) {
								syslog( LOG_NOTICE, "%s: can't create logfile '%s'.\n", judgecode, temp);
							}
							else {
								res = fwrite(msg_begin, sizeof(char), strlen(msg_begin), fp_temp);
								if ( res != strlen(msg_begin))
									syslog( LOG_NOTICE, "%s: write to logfile has a problem - %d\n", config.judgecode, res);
								fclose(fp_temp);
							}
						}
*/

						sprintf(temp, "%s/dip -q", judgedir);
						fp_dipc = popen(temp, "w");
						if(fp_dipc == NULL) syslog( LOG_NOTICE, "%s: No conection to dip\n", judgecode);
						else {
							res = fwrite(msg_begin, sizeof(char), strlen(msg_begin), fp_dipc);
							if ( res != strlen(msg_begin))
								syslog( LOG_NOTICE, "%s: write to dip has a problem - %d\n", judgecode, res);
							pclose(fp_dipc);
						}
						time(&now);
						sprintf(msg.text, "From atrun %ld", now + 61);
						msg.prio=1;
						msgsnd(msgid, &msg, MSGLEN, 0);
					}

					if (msg_end == msgbuffer) msg_begin = NULL;
					else msg_begin = msg_end;
				}

				semaphore_operation (semid, DIP, UNLOCK);
				if (msgbuffer) free(msgbuffer);
				return EXIT_SUCCESS;
			}
		}
	}

	syslog(LOG_NOTICE, "%s: 'SIGQUIT' received.", judgecode);

// waiting for all childs

	j = 0;
	while (childs > 0) {
		res = del_child(childs_pid, fifochilds);
		if (res < 0) syslog(LOG_NOTICE, "%s: error while waitpid. errorcode: %d\n", judgecode, res);
		else if (res > 0) {
			syslog(LOG_NOTICE, "%s: fifo-child '%d' ended.\n", judgecode, res);
			childs--;
			syslog(LOG_NOTICE, "%s: wait for %d childs.\n", judgecode, childs);
		}

		if (j < 1) {
			for (i = 0 ; i < fifochilds ; i++) {
				if (childs_pid[i] > 0) {
						syslog(LOG_NOTICE, "%s: sending SIGQUIT to '%d'\n", judgecode, childs_pid[i]);
						kill (childs_pid[i], 3);
				}
			}
		}

		if (j == 20) {
			for (i = 0 ; i < fifochilds ; i++) {
				if (childs_pid[i] > 0) {
						syslog(LOG_NOTICE, "%s: sending SIGTERM to '%d'\n", judgecode, childs_pid[i]);
						kill (childs_pid[i], 15);
				}
			}
		}

		usleep(100000);
		j++;
	}

	closelog();
	return EXIT_SUCCESS;
}
