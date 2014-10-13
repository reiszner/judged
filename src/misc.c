/***************************************************************************
 *            misc.c
 *
 *  Die Juli 02 01:33:57 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
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



void strtolower (char *text) {
	int i = 0;
	for (i = 0 ; text[i] != '\0' ; i++) text[i] = tolower(text[i]);
}



int chowngrp(int uid, int gid) {
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



/*
0 	LOG_EMERG 	System ist unbrauchbar. 
1 	LOG_ALERT 	Dringend irgendwelche Aktionen einleiten 
2 	LOG_CRIT 	Kritische Nachrichten 
3 	LOG_ERR 	Normaler Fehler 
4 	LOG_WARNING 	Warnung 
5 	LOG_NOTICE 	Normale, bedeutende Nachricht (Standard) 
6 	LOG_INFO 	Normale Nachricht 
7 	LOG_DEBUG 	Unwichtige Nachricht
*/

void logging(const int lvl, const wchar_t *format, ...) {
	wchar_t output[1024];
	va_list arglist;
	va_start(arglist,format);
	vswprintf(output, 1024, format, arglist);

	if (atoi(getenv("JUDGE_DAEMON"))) syslog( lvl, "%ls", output);
	else {
		switch(lvl) {
			case 0:
				fwprintf(stderr, L"Emergency: %ls", output);
				break;
			case 1:
				fwprintf(stderr, L"Alert: %ls", output);
				break;
			case 2:
				fwprintf(stderr, L"Critical: %ls", output);
				break;
			case 3:
				fwprintf(stderr, L"Error: %ls", output);
				break;
			case 4:
				fwprintf(stdout, L"Warning: %ls", output);
				break;
			case 5:
				fwprintf(stdout, L"Notice: %ls", output);
				break;
			case 6:
				fwprintf(stdout, L"Info: %ls", output);
				break;
			case 7:
				fwprintf(stdout, L"Debug: %ls", output);
				break;
			default:
				fwprintf(stdout, L"Unknown: %ls", output);
		}
	}
	va_end(arglist);
}
