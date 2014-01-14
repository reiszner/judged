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
#include <sys/stat.h>
#include <sys/types.h>

#include "fifo.h"
#include "child.h"
#include "ipc.h"
#include "misc.h"

static int run = 1;
static void fifo_quit (int signr) {
	run = 0;
	return;
}

static void fifo_end (int signr) {
	exit(0);
}

int main(int argc, char **argv) {

	struct message msg;

	char judgedir[255], judgecode[255], gateway[255], fifofile[255], string_out[1024];
	int semid, judgeuid, judgegid, fifochilds = 0, childs = 0, judgedaemon, res, i;
	pid_t pid;
	key_t msg_key, sem_key;

	sscanf(getenv("JUDGE_UID"), "%d", &judgeuid);
	sscanf(getenv("JUDGE_GID"), "%d", &judgegid);
	strcpy(judgedir, getenv("JUDGE_DIR"));
	strcpy(judgecode, getenv("JUDGE_CODE"));
	strcpy(gateway, getenv("JUDGE_GATEWAY"));
	strcpy(fifofile, getenv("JUDGE_FIFO"));
	sscanf(getenv("JUDGE_FIFOCHILDS"), "%d", &fifochilds);
	sscanf(getenv("JUDGE_DAEMON"), "%d", &judgedaemon);
	sscanf(getenv("JUDGE_IPCKEY"), "%d", &msg_key);

	pid_t childs_pid[fifochilds];
	for (i = 0; i < fifochilds ; i++) childs_pid[i] = 0;

	if (judgedaemon > 0) {
		openlog( argv[0], LOG_PID | LOG_CONS | LOG_NDELAY, LOG_DAEMON );
	}

	if (strlen(fifofile) > 0) {
		umask (0111);
		if (create_fifo(fifofile)) {
			sprintf( string_out, "%s: couldn't create fifo '%s'.\n", judgecode, fifofile);
			output(LOG_ERR, string_out);
			return EXIT_FAILURE;
		}
	} else {
		sprintf( string_out, "%s: no FIFO stated.\n", judgecode);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}

	if (chown(fifofile, judgeuid, judgegid) != 0) {
		sprintf( string_out, "%s: can't change user and/or group of fifo '%s'. exit.\n", judgecode, fifofile);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}

	if ((res = chowngrp(judgeuid, judgegid)) != 0) {
		if (res == -1) sprintf( string_out, "%s: can't change user. exit.\n", judgecode);
		if (res == -2) sprintf( string_out, "%s: can't change group. exit.\n", judgecode);
		if (res < 0) {
			output(LOG_ERR, string_out);
			return EXIT_FAILURE;
		}
	}

	sem_key = ftok(fifofile, 1);
	if(sem_key == -1) {
		sprintf( string_out, "%s: ftok failed with errno = %d\n", judgecode, errno);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}
	semid = init_semaphore (sem_key);
	if (semid < 0) {
		sprintf( string_out, "%s: couldn't greate FIFO-SemaphoreID.\n", judgecode);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}
	if (semctl (semid, 0, SETVAL, (int) 1) == -1) {
		sprintf( string_out, "%s: couldn't initialize FIFO-SemaphoreID.\n", judgecode);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}

	signal (SIGQUIT, fifo_quit);
	signal (SIGTERM, fifo_quit);
	while(run) {

		if (childs > 0) {
			res = del_child(childs_pid, fifochilds);
			if (res < 0) {
				sprintf( string_out, "%s: error while waitpid. errorcode: %d\n", judgecode, res);
				output(LOG_NOTICE, string_out);
			}
			else if (res > 0) {
				sprintf( string_out, "%s: fifo-child '%d' ended.\n", judgecode, res);
				output(LOG_NOTICE, string_out);
				childs--;
			}
		}

// check if we have all childs running

		if (childs < fifochilds) {

			if ((pid = fork ()) < 0) {
				sprintf( string_out, "%s: error while fork fifo-child.\n", judgecode);
				output(LOG_ERR, string_out);
				run = 0;
			}

/* Parentprocess */
			else if (pid > 0) {
				res = add_child(pid, childs_pid, fifochilds);
				if (res > 0) {
					sprintf( string_out, "%s: fifo-child %d forked with PID '%d'.\n", judgecode, ++childs, res);
					output(LOG_NOTICE, string_out);
				}
			}

/* Childprocess */
			else {

				signal (SIGTERM, fifo_end);

				int msgid, shmid, own_pid, partner, cnt = 0, blocks = 1, pos = 0;
				FILE *fp_fifo;
				char *msgbuffer, answer[128];
				key_t shm_key = -1;
				void *shmdata;
				srand(time(NULL));

				own_pid = getpid();
				msgid = get_msgqueue(msg_key);
				if (msgid < 0) return EXIT_FAILURE;

				semaphore_operation(semid, 0, LOCK);

				if((fp_fifo = fopen(fifofile, "r")) == NULL) {
					sprintf( string_out, "%s: an error occured while opening the fifo '%s'.\n", judgecode, fifofile);
					output(LOG_ERR, string_out);
					semaphore_operation(semid, 0, UNLOCK);
					return EXIT_FAILURE;
				}

				msgbuffer = malloc(BUFFER_SIZE);

				cnt = fread(msgbuffer, sizeof(char), BUFFER_SIZE, fp_fifo);
				signal (SIGTERM, fifo_quit);
				pos += cnt;
				cnt = 0;

				while (feof(fp_fifo) == 0) {
					if ( (int)(pos / BUFFER_SIZE) == blocks) {
						sprintf( string_out, "%s: pos = %d ; realloc to size %d (%d blocks).\n", judgecode, pos, BUFFER_SIZE * (blocks + 1), blocks + 1);
						output(LOG_NOTICE, string_out);
						msgbuffer = realloc(msgbuffer, sizeof(char) * BUFFER_SIZE * ++blocks);
					}
					cnt = fread(&msgbuffer[pos * sizeof(char)], sizeof(char), BUFFER_SIZE, fp_fifo);
					pos += cnt;
					cnt = 0;
				}
				msgbuffer[pos] = 0;
				clearerr(fp_fifo);
				fclose(fp_fifo);

				while (shm_key < 0) shm_key = time(NULL)-rand();
				shmid = init_sharedmemory(shm_key);
				shmdata = shmat(shmid, NULL, 0);

				msg.prio=1;
				sprintf( msg.text, "MESSAGE %d\n", own_pid);
				msgsnd(msgid, &msg, MSGLEN, 0);

				cnt = 1;
				while (cnt) {
					msgrcv(msgid, &msg, MSGLEN, own_pid, 0);

					if (strncmp("READY", msg.text, 5) == 0) {
						sscanf(msg.text + 5,"%d", &partner);
						msg.prio=partner;
						sprintf( msg.text, "SHM_ID %d\n", shmid);
						msgsnd(msgid, &msg, MSGLEN, 0);
					}

					else if (strncmp("SHM_OK", msg.text, 6) == 0) {
						pos = send_sharedmemory(shmdata, msgbuffer, msgid, own_pid, partner);
						if (pos) {
							sprintf(string_out, "%s: send per sharedmemory apported.\n", judgecode);
							output(LOG_ERR, string_out);
							cnt = 0;
						}
						else {
							msg.prio=partner;
							sprintf( msg.text, "SHM_WAIT\n");
							msgsnd(msgid, &msg, MSGLEN, 0);
						}
						free(msgbuffer);
						msgbuffer = NULL;
					}

					else if (strncmp("SHM_ANSWER", msg.text, 10) == 0) {
						sscanf(msg.text + 10,"%s", answer);
						msgbuffer = resive_sharedmemory(shmdata, msgid, own_pid, partner);
						if (msgbuffer == NULL) {
							sprintf(string_out, "%s: resive per sharedmemory apported.\n", judgecode);
							output(LOG_ERR, string_out);
						}
					}

					else if (strncmp("SHM_BYE", msg.text, 7) == 0) cnt = 0;

					else {
						sprintf(string_out, "%s: don't understand '%s'\n", judgecode, msg.text);
						output(LOG_ERR, string_out);
						cnt = 0;
					}
				}

				if(shmdata) {
					shmdt(shmdata);
					shmctl(shmid, IPC_RMID, 0);
				}

// Antworten !!!

				if (msgbuffer) {
					if((fp_fifo = fopen(answer, "w")) == NULL) {
						sprintf(string_out, "%s: an error occured while opening the fifo '%s'.\n", judgecode, answer);
						output(LOG_ERR, string_out);
						return EXIT_FAILURE;
					}

					cnt = fwrite(msgbuffer, sizeof(char), strlen(msgbuffer), fp_fifo);
					if (cnt != strlen(msgbuffer)) {
						sprintf(string_out, "%s: an error occured while write to fifo '%s'.\n", judgecode, answer);
						output(LOG_ERR, string_out);
					}

					clearerr(fp_fifo);
					fclose(fp_fifo);
					free(msgbuffer);
				}

				return EXIT_SUCCESS;
			}
		}
		usleep (100000);
	}

// waiting for all childs
	for (i = 0 ; i < fifochilds ; i++) {
		if (childs_pid[i] > 0) {
			sprintf( string_out, "%s: sending SIGTERM to '%d'\n", judgecode, childs_pid[i]);
			output(LOG_NOTICE, string_out);
			kill (childs_pid[i], 15);
		}
	}

	while (childs > 0) {
		res = del_child(childs_pid, fifochilds);
		if (res < 0) {
			sprintf( string_out, "%s: error while waitpid. errorcode: %d\n", judgecode, res);
			output(LOG_NOTICE, string_out);
		}
		else if (res > 0) {
			sprintf( string_out, "%s: fifo-child '%d' ended.\n", judgecode, res);
			output(LOG_NOTICE, string_out);
			childs--;
		}
		usleep(100000);
	}

	closelog();
	return EXIT_SUCCESS;
}
