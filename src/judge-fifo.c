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

	char judgecode[255], fifofile[255];
	int semid, fifochilds = 0, childs = 0, judgedaemon, res, i;
	pid_t pid;
	key_t msg_key, sem_key;

	strcpy(judgecode, getenv("JUDGE_CODE"));
	strcpy(fifofile, getenv("JUDGE_FIFO"));
	sscanf(getenv("JUDGE_FIFOCHILDS"), "%d", &fifochilds);
	sscanf(getenv("JUDGE_DAEMON"), "%d", &judgedaemon);
	sscanf(getenv("JUDGE_IPCKEY"), "%d", &msg_key);

	pid_t childs_pid[fifochilds];
	for (i = 0; i < fifochilds ; i++) childs_pid[i] = 0;

	if (judgedaemon > 0) {
		openlog( argv[0], LOG_PID | LOG_CONS | LOG_NDELAY, LOG_DAEMON );
	}

	sem_key = ftok(fifofile, 1);
	if(sem_key == -1) {
		logging(LOG_ERR, L"%s: ftok failed with errno = %d\n", judgecode, errno);
		return EXIT_FAILURE;
	}
	semid = init_semaphore (sem_key);
	if (semid < 0) {
		logging(LOG_ERR, L"%s: couldn't greate FIFO-SemaphoreID.\n", judgecode);
		return EXIT_FAILURE;
	}
	if (semctl (semid, 0, SETVAL, (int) 1) == -1) {
		logging(LOG_ERR, L"%s: couldn't initialize FIFO-SemaphoreID.\n", judgecode);
		return EXIT_FAILURE;
	}

	signal (SIGQUIT, fifo_quit);
	signal (SIGTERM, fifo_quit);
	while(run) {

		if (childs > 0) {
			res = del_child(childs_pid, fifochilds);
			if (res < 0) {
				logging(LOG_NOTICE, L"%s: error while waitpid. errorcode: %d\n", judgecode, res);
			}
			else if (res > 0) {
				logging(LOG_NOTICE, L"%s: fifo-child '%d' ended.\n", judgecode, res);
				childs--;
			}
		}

// check if we have all childs running

		if (childs < fifochilds) {

			if ((pid = fork ()) < 0) {
				logging(LOG_ERR, L"%s: error while fork fifo-child.\n", judgecode);
				run = 0;
			}

/* Parentprocess */
			else if (pid > 0) {
				res = add_child(pid, childs_pid, fifochilds);
				if (res > 0) {
					logging(LOG_NOTICE, L"%s: fifo-child %d forked with PID '%d'.\n", judgecode, ++childs, res);
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
					logging(LOG_ERR, L"%s: an error occured while opening the fifo '%s'.\n", judgecode, fifofile);
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
						logging(LOG_NOTICE, L"%s: pos = %d ; realloc to size %d (%d blocks).\n", judgecode, pos, BUFFER_SIZE * (blocks + 1), blocks + 1);
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
				send_msg (msgid, 0, 1, L"MESSAGE %d", own_pid);

				cnt = 1;
				while (cnt) {
					msgrcv(msgid, &msg, MSGLEN, own_pid, 0);

					if (wcsncmp(L"READY", msg.text, 5) == 0) {
						swscanf(msg.text + 5, L"%d", &partner);
						send_msg (msgid, 0, partner, L"SHM_ID %d", shmid);
					}

					else if (wcsncmp(L"SHM_OK", msg.text, 6) == 0) {
						pos = send_sharedmemory(shmdata, msgbuffer, msgid, own_pid, partner);
						if (pos) {
							logging(LOG_ERR, L"%s: send per sharedmemory apported.\n", judgecode);
							cnt = 0;
						}
						else send_msg (msgid, 0, partner, L"SHM_WAIT");
						free(msgbuffer);
						msgbuffer = NULL;
					}

					else if (wcsncmp(L"SHM_ANSWER", msg.text, 10) == 0) {
						swscanf(msg.text + 10, L"%s", answer);
						msgbuffer = resive_sharedmemory(shmdata, msgid, own_pid, partner);
						if (msgbuffer == NULL) {
							logging(LOG_ERR, L"%s: resive per sharedmemory apported.\n", judgecode);
						}
					}

					else if (wcsncmp(L"SHM_BYE", msg.text, 7) == 0) cnt = 0;

					else {
						logging(LOG_ERR, L"%s: don't understand '%s'\n", judgecode, msg.text);
						cnt = 0;
					}
				}

				if(shmdata) {
					shmdt(shmdata);
					shmctl(shmid, IPC_RMID, NULL);
				}

// Antworten !!!

				if (msgbuffer) {
					if((fp_fifo = fopen(answer, "w")) == NULL) {
						logging(LOG_ERR, L"%s: an error occured while opening the fifo '%s'.\n", judgecode, answer);
						return EXIT_FAILURE;
					}

					cnt = fwrite(msgbuffer, sizeof(char), strlen(msgbuffer), fp_fifo);
					if (cnt != strlen(msgbuffer)) {
						logging(LOG_ERR, L"%s: an error occured while write to fifo '%s'.\n", judgecode, answer);
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
			logging(LOG_NOTICE, L"%s: sending SIGTERM to '%d'\n", judgecode, childs_pid[i]);
			kill (childs_pid[i], 15);
		}
	}

	while (childs > 0) {
		res = del_child(childs_pid, fifochilds);
		if (res < 0) {
			logging(LOG_NOTICE, L"%s: error while waitpid. errorcode: %d\n", judgecode, res);
		}
		else if (res > 0) {
			logging(LOG_NOTICE, L"%s: fifo-child '%d' ended.\n", judgecode, res);
			childs--;
		}
		usleep(100000);
	}

	closelog();
	return EXIT_SUCCESS;
}
