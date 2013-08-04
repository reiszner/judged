/***************************************************************************
 *            misc.c
 *
 *  Die Juli 02 01:33:57 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "misc.h"

sigfunk *signal (int sig_nr, sigfunk signalhandler) {
	struct sigaction neu_sig, alt_sig;
	neu_sig.sa_handler = signalhandler;
	sigemptyset (&neu_sig.sa_mask);
	neu_sig.sa_flags = SA_RESTART;
	if (sigaction (sig_nr, &neu_sig, &alt_sig) < 0)
		return SIG_ERR;
	return alt_sig.sa_handler;
}

void strtolower (char *text)
{
	int i = 0;
	for (i = 0 ; text[i] != '\0' ; i++) text[i] = tolower(text[i]);
}

int chowngrp(int uid, int gid)
{
	if (getgid() != gid) {
		if ((setgid(gid)) != 0) {
			return -2;
		}
	}

	if (getuid() != uid) {
		if ((setuid(uid)) != 0) {
			return -1;
		}
	}
	return 0;
}
