/***************************************************************************
 *            judge-socket.c
 *
 *  Mit Juli 10 11:31:28 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <syslog.h>

#include "ipc.h"
#include "socket.h"
#include "misc.h"

#define NONE 0
#define UNIX 1
#define INET 2

static int run = 1;

static void socket_quit (int signr) {
	run = 0;
}



int main(int argc, char **argv) {

	struct message msg;

	char judgedir[255], judgecode[255], gateway[255], socket[255];
	int judgeuid, judgegid, childs = 0, type = NONE, status = 0, port;
	int fd_socket, fd_connect;
	key_t ipc_key;

	sscanf(getenv("JUDGE_UID"), "%d", &judgeuid);
	sscanf(getenv("JUDGE_GID"), "%d", &judgegid);
	strcpy(judgedir, getenv("JUDGE_DIR"));
	strcpy(judgecode, getenv("JUDGE_CODE"));
	strcpy(gateway, getenv("JUDGE_GATEWAY"));
	sscanf(getenv("JUDGE_IPCKEY"), "%d", &ipc_key);

	openlog( argv[0], LOG_PID | LOG_CONS | LOG_NDELAY, LOG_DAEMON );

// looking for type of socket

	if (getenv("JUDGE_INET") != NULL) {
		syslog(LOG_NOTICE, "%s: inet-socket found.\n", judgecode);
		strcpy(socket, getenv("JUDGE_INET"));
		sscanf(getenv("JUDGE_INETPORT"), "%d", &port);
		type = INET;
	}
	else if (getenv("JUDGE_UNIX") != NULL) {
		syslog(LOG_NOTICE, "%s: unix-socket found.\n", judgecode);
		strcpy(socket, getenv("JUDGE_UNIX"));
		type = UNIX;
	}

	if(type == NONE) {
		syslog(LOG_NOTICE, "%s: no socket defined. exit.\n", judgecode);
		return EXIT_FAILURE;
	}

// create the socket

	umask (0111);
	if (type == UNIX) {
		if ((fd_socket = create_socket(AF_LOCAL, SOCK_STREAM, 0)) == 0) {
			syslog(LOG_NOTICE, "%s: couldn't create unix-socket '%s'.\n", judgecode, socket);
			status = -1;
		}
		else {
			if (bind_un_socket(&fd_socket, socket) != 0) {
				status = -2;
				syslog(LOG_NOTICE, "%s: unix-socket '%s' isn't free. exit.\n", judgecode, socket);
			}
			else {
				if (chown(socket, judgeuid, judgegid) != 0) {
					status = -3;
					syslog(LOG_NOTICE, "%s: can't change user and/or group of unix-socket '%s'. exit.\n", judgecode, socket);
				}
			}
		}
	}
	if (type == INET) {
		if ((fd_socket = create_socket(AF_INET, SOCK_STREAM, 0)) == 0) {
			syslog(LOG_NOTICE, "%s: couldn't create inet-socket '%s:%d'.\n", judgecode, socket, port);
			status = -1;
		}
		else {
			if (bind_in_socket(&fd_socket, socket, port) != 0) {
				status = -2;
				syslog(LOG_NOTICE, "%s: inet-socket '%s:%d' isn't free. exit.\n", judgecode, socket, port);
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

// listen on socket

	if (listen (fd_socket, SOMAXCONN) != 0) {
		syslog(LOG_NOTICE, "%s: can't listen on socket '%s'. exit.\n", judgecode, socket);
		return EXIT_FAILURE;
	}

// accept connections until quit

	signal (SIGQUIT, socket_quit);
	while (run) {
		usleep (100000);
//		while ((fd_connect = accept_socket(&fd_socket)) >= 0) {

	}

	syslog(LOG_NOTICE, "%s: 'SIGQUIT' received.", judgecode);
	close_socket(&fd_socket);
	return EXIT_SUCCESS;
}
