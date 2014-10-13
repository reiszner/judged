/***************************************************************************
 *            judge-sock.c
 *
 *  Mit Juli 10 11:31:28 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <errno.h>
#include <time.h>
#include <sys/wait.h>

#include "ipc.h"
#include "socket.h"
#include "misc.h"

#define NONE 0
#define UNIX 1
#define INET 2

static int run = 1;

static void socket_quit (int signr) {
	run = 0;
	return;
}

static void child_quit (int signr) {
	pid_t pid;
	while ((pid = waitpid (-1, NULL, WNOHANG)) > 0);
	return;
}



int main(int argc, char **argv) {

	char judgecode[255], socket[255];
	int judgeuid, judgegid, type = NONE, status = 0, port, judgedaemon, res;
	int fd_socket, fd_connect;
	pid_t pid;
	key_t msg_key;

	sscanf(getenv("JUDGE_UID"), "%d", &judgeuid);
	sscanf(getenv("JUDGE_GID"), "%d", &judgegid);
	strcpy(judgecode, getenv("JUDGE_CODE"));
	sscanf(getenv("JUDGE_DAEMON"), "%d", &judgedaemon);
	sscanf(getenv("JUDGE_IPCKEY"), "%d", &msg_key);

	openlog( argv[0], LOG_PID | LOG_CONS | LOG_NDELAY, LOG_DAEMON );

// looking for type of socket

	if (getenv("JUDGE_INET") != NULL) {
		logging(LOG_NOTICE, L"%s: inet-socket found.\n", judgecode);
		strcpy(socket, getenv("JUDGE_INET"));
		sscanf(getenv("JUDGE_INETPORT"), "%d", &port);
		type = INET;
	}
	else if (getenv("JUDGE_UNIX") != NULL) {
		logging(LOG_NOTICE, L"%s: unix-socket found.\n", judgecode);
		strcpy(socket, getenv("JUDGE_UNIX"));
		type = UNIX;
	}

	if(type == NONE) {
		logging(LOG_ERR, L"%s: no socket defined. exit.\n", judgecode);
		return EXIT_FAILURE;
	}

// create the socket

	umask (0111);
	if (type == UNIX) {
		if ((fd_socket = create_socket(AF_LOCAL, SOCK_STREAM, 0)) == 0) {
			logging(LOG_ERR, L"%s: couldn't create unix-socket '%s'.\n", judgecode, socket);
			status = -1;
		}
		else {
			if (bind_un_socket(&fd_socket, socket) != 0) {
				status = -2;
				logging(LOG_ERR, L"%s: unix-socket '%s' isn't free. exit.\n", judgecode, socket);
			}
			else {
				if (chown(socket, judgeuid, judgegid) != 0) {
					status = -3;
					logging(LOG_ERR, L"%s: can't change user and/or group of unix-socket '%s'. exit.\n", judgecode, socket);
				}
			}
		}
	}
	if (type == INET) {
		if ((fd_socket = create_socket(AF_INET, SOCK_STREAM, 0)) == 0) {
			logging(LOG_ERR, L"%s: couldn't create inet-socket '%s:%d'.\n", judgecode, socket, port);
			status = -1;
		}
		else {
			if (bind_in_socket(&fd_socket, socket, port) != 0) {
				status = -2;
				logging(LOG_ERR, L"%s: inet-socket '%s:%d' isn't free. exit.\n", judgecode, socket, port);
			}
		}
	}

// exit if there was a problem with socket

	if (status < 0) {
		if (status < -1) close_socket(&fd_socket);
		if (status < -2 && type == UNIX) unlink(socket);
		return EXIT_FAILURE;
	}

/*
 *
 * change user and group
 *
 */

	if ((res = chowngrp(judgeuid, judgegid)) != 0) {
		if (res == -1) logging(LOG_ERR, L"%s: can't change user. exit.\n", judgecode);
		if (res == -2) logging(LOG_ERR, L"%s: can't change group. exit.\n", judgecode);
		if (res < 0) return EXIT_FAILURE;
	}

// listen on socket

	if (listen (fd_socket, SOMAXCONN) != 0) {
		logging(LOG_ERR, L"%s: can't listen on socket '%s'. exit.\n", judgecode, socket);
		return EXIT_FAILURE;
	}

// accept connections until quit

	signal (SIGQUIT, socket_quit);
	signal (SIGCHLD, child_quit);
	while (run) {

		fd_connect = accept_socket(&fd_socket);
		if( fd_connect < 0 ) {
			if( errno == EINTR )
				continue;
			else {
				logging(LOG_ERR, L"%s: error while accept() (%s)\n", judgecode, strerror(errno));
				return EXIT_FAILURE;
			}
		}

		if ((pid = fork ()) < 0) {
			logging(LOG_ERR, L"%s: error while fork sock-child.\n", judgecode);
			run = 0;
		}

/* Parentprocess */
		else if (pid > 0) {
			close (fd_connect);
		}

/* Childprocess */
		else {
			close (fd_socket);

			int msgid, shmid, own_pid, partner, cnt = 0, blocks = 1, pos = 0;
			char *msgbuffer;
			key_t shm_key = -1;
			void *shmdata;
			srand(time(NULL));
			struct message msg;

			own_pid = getpid();
			msgid = msgget (msg_key, IPC_PRIVATE);
			if (msgid < 0) return EXIT_FAILURE;

			msgbuffer = malloc(BUFFER_SIZE);
			while ((cnt = read(fd_connect, &msgbuffer[pos * sizeof(char)], BUFFER_SIZE * sizeof(char) * blocks - pos)) > 0) {
				pos += cnt;
				cnt = 0;
				if ( (int)(pos / BUFFER_SIZE) == blocks) {
					logging(LOG_NOTICE, L"%s: pos = %d ; realloc to size %d (%d blocks).\n", judgecode, pos, BUFFER_SIZE * (blocks + 1), blocks + 1);
					msgbuffer = realloc(msgbuffer, BUFFER_SIZE * sizeof(char) * ++blocks);
				}
			}
			msgbuffer[pos] = 0;

			while (shm_key < 0) shm_key = time(NULL)-rand();
			shmid = init_sharedmemory (shm_key);
			shmdata = shmat(shmid, NULL, 0);
			send_msg (msgid, 0, 1, L"MESSAGE %d\0", own_pid);

			cnt = 1;
			while (cnt) {
				msgrcv(msgid, &msg, MSGLEN, own_pid, 0);

				if (wcsncmp(L"READY", msg.text, 5) == 0) {
					swscanf(msg.text + 5, L"%d", &partner);
					send_msg (msgid, 0, partner, L"SHM_ID %d\0", shmid);
				}

				else if (wcsncmp(L"SHM_OK", msg.text, 6) == 0) {
					pos = send_sharedmemory(shmdata, msgbuffer, msgid, own_pid, partner);
					if (pos) {
						logging(LOG_ERR, L"%s: send per sharedmemory apported.\n", judgecode);
						cnt = 0;
					}
					else send_msg (msgid, 0, partner, L"SHM_WAIT\0");
					free(msgbuffer);
					msgbuffer = NULL;
				}

				else if (wcsncmp(L"SHM_ANSWER", msg.text, 10) == 0) {
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

			if (shmdata) {
				shmdt(shmdata);
				shmctl(shmid, IPC_RMID, NULL);
			}

// Antworten !!!

			if (msgbuffer) {
				cnt = write(fd_connect, msgbuffer, strlen(msgbuffer));
				if (cnt != strlen(msgbuffer)) logging(LOG_ERR, L"%s: an error occured while write to socket.\n", judgecode);

				close(fd_connect);
				free(msgbuffer);
			}

			return EXIT_SUCCESS;
		}

	}

	logging(LOG_NOTICE, L"%s: 'SIGQUIT' received.", judgecode);
	close_socket(&fd_socket);
	return EXIT_SUCCESS;
}
