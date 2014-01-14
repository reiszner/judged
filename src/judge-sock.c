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

	struct message msg;

	char judgedir[255], judgecode[255], gateway[255], socket[255], string_out[1024];
	int judgeuid, judgegid, type = NONE, status = 0, port, judgedaemon, res;
	int fd_socket, fd_connect;
	pid_t pid;
	key_t msg_key;

	sscanf(getenv("JUDGE_UID"), "%d", &judgeuid);
	sscanf(getenv("JUDGE_GID"), "%d", &judgegid);
	strcpy(judgedir, getenv("JUDGE_DIR"));
	strcpy(judgecode, getenv("JUDGE_CODE"));
	strcpy(gateway, getenv("JUDGE_GATEWAY"));
	sscanf(getenv("JUDGE_DAEMON"), "%d", &judgedaemon);
	sscanf(getenv("JUDGE_IPCKEY"), "%d", &msg_key);

	openlog( argv[0], LOG_PID | LOG_CONS | LOG_NDELAY, LOG_DAEMON );

// looking for type of socket

	if (getenv("JUDGE_INET") != NULL) {
		sprintf( string_out, "%s: inet-socket found.\n", judgecode);
		output(LOG_NOTICE, string_out);
		strcpy(socket, getenv("JUDGE_INET"));
		sscanf(getenv("JUDGE_INETPORT"), "%d", &port);
		type = INET;
	}
	else if (getenv("JUDGE_UNIX") != NULL) {
		sprintf( string_out, "%s: unix-socket found.\n", judgecode);
		output(LOG_NOTICE, string_out);
		strcpy(socket, getenv("JUDGE_UNIX"));
		type = UNIX;
	}

	if(type == NONE) {
		sprintf( string_out, "%s: no socket defined. exit.\n", judgecode);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}

// create the socket

	umask (0111);
	if (type == UNIX) {
		if ((fd_socket = create_socket(AF_LOCAL, SOCK_STREAM, 0)) == 0) {
			sprintf( string_out, "%s: couldn't create unix-socket '%s'.\n", judgecode, socket);
			output(LOG_ERR, string_out);
			status = -1;
		}
		else {
			if (bind_un_socket(&fd_socket, socket) != 0) {
				status = -2;
				sprintf( string_out, "%s: unix-socket '%s' isn't free. exit.\n", judgecode, socket);
				output(LOG_ERR, string_out);
			}
			else {
				if (chown(socket, judgeuid, judgegid) != 0) {
					status = -3;
					sprintf( string_out, "%s: can't change user and/or group of unix-socket '%s'. exit.\n", judgecode, socket);
					output(LOG_ERR, string_out);
				}
			}
		}
	}
	if (type == INET) {
		if ((fd_socket = create_socket(AF_INET, SOCK_STREAM, 0)) == 0) {
			sprintf( string_out, "%s: couldn't create inet-socket '%s:%d'.\n", judgecode, socket, port);
			output(LOG_ERR, string_out);
			status = -1;
		}
		else {
			if (bind_in_socket(&fd_socket, socket, port) != 0) {
				status = -2;
				sprintf( string_out, "%s: inet-socket '%s:%d' isn't free. exit.\n", judgecode, socket, port);
				output(LOG_ERR, string_out);
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
		if (res == -1) sprintf( string_out, "%s: can't change user. exit.\n", judgecode);
		if (res == -2) sprintf( string_out, "%s: can't change group. exit.\n", judgecode);
		if (res < 0) {
			return EXIT_FAILURE;
			output(LOG_ERR, string_out);
		}
	}

// listen on socket

	if (listen (fd_socket, SOMAXCONN) != 0) {
		sprintf( string_out, "%s: can't listen on socket '%s'. exit.\n", judgecode, socket);
		output(LOG_ERR, string_out);
		return EXIT_FAILURE;
	}

// accept connections until quit

	signal (SIGQUIT, socket_quit);
	signal (SIGCHLD, child_quit);
	while (run) {
/*
		pid = waitpid (-1, NULL, WNOHANG);
		if (pid < 0) syslog(LOG_NOTICE, "%s: error while waitpid. errorcode: %d\n", judgecode, pid);
		else if (pid > 0) {
			syslog(LOG_NOTICE, "%s: sock-child '%d' ended.\n", judgecode, pid);
		}
*/
		fd_connect = accept_socket(&fd_socket);
		if( fd_connect < 0 ) {
			if( errno == EINTR )
				continue;
			else {
				sprintf( string_out, "%s: error while accept() (%s)\n", judgecode, strerror(errno));
				output(LOG_ERR, string_out);
				return EXIT_FAILURE;
			}
		}

		if ((pid = fork ()) < 0) {
			sprintf( string_out, "%s: error while fork sock-child.\n", judgecode);
			output(LOG_ERR, string_out);
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

			own_pid = getpid();
			msgid = msgget (msg_key, IPC_PRIVATE);
			if (msgid < 0) return EXIT_FAILURE;

/*
			sprintf(temp, "Judged: %s\n", judgecode);
			cnt = strlen(temp);
			if ( write(fd_connect, temp, cnt) != cnt) {
				syslog( LOG_NOTICE, "%s: error while write()...(%s)\n", judgecode, strerror(errno));
				return EXIT_FAILURE;
			}

			cnt = read(fd_connect, temp, sizeof(temp));
			temp[cnt] = 0;

			if (strncmp("message", temp, 7) == 0) {
*/

			msgbuffer = malloc(BUFFER_SIZE);
			while ((cnt = read(fd_connect, &msgbuffer[pos * sizeof(char)], BUFFER_SIZE * sizeof(char) * blocks - pos)) > 0) {
				pos += cnt;
				cnt = 0;
				if ( (int)(pos / BUFFER_SIZE) == blocks) {
					sprintf( string_out, "%s: pos = %d ; realloc to size %d (%d blocks).\n", judgecode, pos, BUFFER_SIZE * (blocks + 1), blocks + 1);
					output(LOG_NOTICE, string_out);
					msgbuffer = realloc(msgbuffer, BUFFER_SIZE * sizeof(char) * ++blocks);
				}
			}
			msgbuffer[pos] = 0;

			while (shm_key < 0) shm_key = time(NULL)-rand();
			shmid = init_sharedmemory (shm_key);
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

			if (shmdata) {
				shmdt(shmdata);
				shmctl(shmid, IPC_RMID, 0);
			}

// Antworten !!!

			if (msgbuffer) {
				cnt= write(fd_connect, msgbuffer, strlen(msgbuffer));
				if (cnt != strlen(msgbuffer)) {
					sprintf(string_out, "%s: an error occured while write to socket.\n", judgecode);
					output(LOG_ERR, string_out);
				}

				close(fd_connect);
				free(msgbuffer);
			}

			return EXIT_SUCCESS;
		}

	}

	sprintf( string_out, "%s: 'SIGQUIT' received.", judgecode);
	output(LOG_NOTICE, string_out);
	close_socket(&fd_socket);
	return EXIT_SUCCESS;
}
