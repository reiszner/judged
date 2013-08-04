/***************************************************************************
 *            child.c
 *
 *  Mon Juli 08 20:59:03 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "child.h"

pid_t add_child(pid_t pid, pid_t childs_pid[], int z) {
	int i;
	for (i = 0 ; i < z ; i++) {
		if (childs_pid[i] == 0) {
			childs_pid[i] = pid;
			break;
		}
	}
	return pid;
}

pid_t del_child(pid_t childs_pid[], int z) {
	int i;
	pid_t pid;
	pid = waitpid (-1, NULL, WNOHANG);
	if (pid > 0) {
		for (i = 0 ; i < z ; i++) {
			if (childs_pid[i] == pid) {
				childs_pid[i] = 0;
				break;
			}
		}
	}
	return pid;
}
