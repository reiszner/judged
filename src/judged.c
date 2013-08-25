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
#include <syslog.h>
#include <errno.h>
#include <sys/wait.h>

#include "misc.h"
#include "config.h"
#include "ipc.h"

struct wakeup {
	int mon;
	int day;
	int hrs;
	int min;
};

static int run = 1;
static int conf = 0;

/*
 *
 * signal-function
 *
 */

static void master_term (int signr) {
	syslog(LOG_NOTICE, "%s: SIGTERM received. prepare to quit...\n", config.judgecode);
	run = 0;
}

static void master_conf (int signr) {
	syslog(LOG_NOTICE, "%s: SIGHUP received. reread configfile...\n", config.judgecode);
	conf = 1;
}

/*
 *
 * daemon-function
 *
 */

static void start_daemon (const char *log_name, int facility) {
	int i;
	pid_t pid;
/* ignore SIGHUP */
	signal(SIGHUP, SIG_IGN);
	if ((pid = fork ()) != 0) exit (EXIT_SUCCESS);
	if (setsid() < 0) {
		printf("%s can't set sessionID. exit.\n", log_name);
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

	int run = 0, res, i;
	for (i = 0 ; i < 7 ; i++) {
		if (pid_childs[i] > 0) {
			if (i==0)      syslog(LOG_NOTICE, "%s: sending 'SIGTERM' to fifo-child (%d)\n", config.judgecode, pid_childs[i]);
			else if (i==1) syslog(LOG_NOTICE, "%s: sending 'SIGTERM' to unix-child (%d)\n", config.judgecode, pid_childs[i]);
			else           syslog(LOG_NOTICE, "%s: sending 'SIGTERM' to inet-child (%d)\n", config.judgecode, pid_childs[i]);
			kill (pid_childs[i], SIGTERM);
			run++;
		}
	}

	while (run > 0) {
		res = waitpid (-1, NULL, WNOHANG);
		if (res < 0) syslog(LOG_NOTICE, "%s: error while waitpid. errorcode: %d\n", config.judgecode, res);
		else if (res > 0) {
			for (i = 0 ; i < 7 ; i++) {
				if (res == pid_childs[i]) {
					if (i==0)      syslog(LOG_NOTICE, "%s: fifo-child ended (%d)\n", config.judgecode, pid_childs[i]);
					else if (i==1) syslog(LOG_NOTICE, "%s: unix-child ended (%d)\n", config.judgecode, pid_childs[i]);
					else           syslog(LOG_NOTICE, "%s: inet-child ended (%d)\n", config.judgecode, pid_childs[i]);
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
	syslog( LOG_NOTICE, "%s: ended successfully.\n", config.judgecode);
	closelog();
	return EXIT_SUCCESS;
}

/*****************************************************************************/

int main(int argc, char **argv)
{

	struct message msg;
	struct msqid_ds msgstat;
	struct tm *tmnow;
	struct wakeup tmwake;


	int master_sort = 0, master_mark = 0, semid = 0, msgid = 0, res, i, j;
	key_t ipc_key;
	FILE *fp_dip = NULL, *fp_master, *fp_temp;
	time_t now, wake;
	char gamename[1000][16], buffer[MSGLEN];
	pid_t pid;
	char temp[1024];
	int run = 1;
	pid_t pid_childs[6];

	if ( argc < 2 ) {
		fprintf(stderr, "No configfile specified.\nUsage: %s <configfile>\n",argv[0]);
		return EXIT_FAILURE;
	}
	config.file[0] = '\0';
	strcat(config.file, argv[1]);

/*
 *
 * we go now in background
 *
 */

	start_daemon("judged", LOG_DAEMON);

// read the configfile

	if (read_config(&config) == NULL) {
		syslog( LOG_NOTICE, "%s: error while read configfile '%s'. exit.\n", config.judgecode, config.file);
		return EXIT_FAILURE;
	}

// prepare for childs
/*
	childs=2;

	if (strlen(config.inetsocket) > 0) {
		sprintf(temp, "%s", config.inetsocket);
		ptr = strtok(temp, ", ");
		while(ptr != NULL) {
//			printf("% d. Wort: %s\n", childs, ptr);
			childs++;
			ptr = strtok(NULL, ", ");
		}
	}

	if (strlen(config.fifofile) > 0 && config.fifochilds > 0) pid_childs[0] = 0;
	else pid_childs[0] = -1;

	if (strlen(config.unixsocket) > 0) pid_childs[1] = 0;
	else pid_childs[1] = -1;

	if (strlen(config.inetsocket) > 0) {
		for (i = 2 ; i < childs ; i++) pid_childs[i] = 0;
	}
*/

/*
 *
 * create PID-file
 *
 */

	umask (0133);
	if((fp_temp = fopen(config.pidfile,"r")) == NULL) {
		if((fp_temp = fopen(config.pidfile,"w")) == NULL) {
			syslog( LOG_NOTICE, "%s: can't create pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
			return EXIT_FAILURE;
		}
		else {
			fclose(fp_temp);
			if (chown(config.pidfile, config.judgeuid, config.judgegid) != 0) {
				syslog( LOG_NOTICE, "%s: can't change user and/or group of pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
				return EXIT_FAILURE;
			}
		}
	}
	else {
		syslog( LOG_NOTICE, "%s: existing pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
		return EXIT_FAILURE;
	}





	ipc_key = ftok(config.file, 1);
	if(ipc_key == -1) {
		syslog( LOG_NOTICE, "%s: ftok failed with errno = %d\n", config.judgecode, errno);
		return EXIT_FAILURE;
	}

	semid = init_semaphore (ipc_key);
	if (semid < 0) {
		syslog( LOG_NOTICE, "%s: couldn't greate IPC-SemaphoreID.\n", config.judgecode);
		return EXIT_FAILURE;
	}
	msgid = init_msgqueue (ipc_key);
	if (msgid < 0) {
		syslog( LOG_NOTICE, "%s: couldn't greate IPC-MessageID.\n", config.judgecode);
		return EXIT_FAILURE;
	}

// set enviroment
/*
	if (config.judgeuid > 0) {
		sprintf(temp,"%d",config.judgeuid);
		if( setenv("JUDGE_UID", temp, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_UID\n", config.judgecode);
			return EXIT_FAILURE;
		}
	} else if (config.judgeuid < 0) {
		syslog( LOG_NOTICE, "%s: no User/UID found in configfile. exit.\n", config.judgecode);
		return EXIT_FAILURE;
	}

	if (config.judgegid > 0) {
		sprintf(temp,"%d",config.judgegid);
		if( setenv("JUDGE_GID", temp, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_GID\n", config.judgecode);
			return EXIT_FAILURE;
		}
	} else if (config.judgegid < 0) {
		syslog( LOG_NOTICE, "%s: no Group/GID found in configfile. exit.\n", config.judgecode);
		return EXIT_FAILURE;
	}

	if (strlen(config.judgedir) > 0) {
		if( setenv("JUDGE_DIR", config.judgedir, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_DIR\n", config.judgecode);
			return EXIT_FAILURE;
		}
	} else {
		syslog( LOG_NOTICE, "%s: no directory found in configfile. exit.\n", config.judgecode);
		return EXIT_FAILURE;
	}

	if (strlen(config.judgecode) > 0) {
		if( setenv("JUDGE_CODE", config.judgecode, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_CODE\n", config.judgecode);
			return EXIT_FAILURE;
		}
	} else {
		syslog( LOG_NOTICE, "%s: no judgecode found in configfile. exit.\n", config.judgecode);
		return EXIT_FAILURE;
	}

	if (strlen(config.ourselves) > 0) {
		if( setenv("JUDGE_SELVE", config.ourselves, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_SELVE\n", config.judgecode);
			return EXIT_FAILURE;
		}
	} else {
		syslog( LOG_NOTICE, "%s: no address for ourselve found in configfile. exit.\n", config.judgecode);
		return EXIT_FAILURE;
	}

	if (strlen(config.judgekeeper) > 0) {
		if( setenv("JUDGE_KEEPER", config.judgekeeper, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_KEEPER\n", config.judgecode);
			return EXIT_FAILURE;
		}
	} else {
		syslog( LOG_NOTICE, "%s: no address for judgekeeper found in configfile. exit.\n", config.judgecode);
		return EXIT_FAILURE;
	}

	if (strlen(config.gateway) > 0) {
		if (setenv("JUDGE_GATEWAY", config.gateway, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_GATEWAY\n", config.judgecode);
			return EXIT_FAILURE;
		}
	} else {
		syslog( LOG_NOTICE, "%s: no address for gateway found in configfile. exit.\n", config.judgecode);
		return EXIT_FAILURE;
	}

	if (strlen(config.fifofile) > 0 && config.fifochilds > 0) {
		sprintf(temp,"%d",config.fifochilds);
		if( setenv("JUDGE_FIFOCHILDS", temp, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_FIFOCHILDS\n", config.judgecode);
			return EXIT_FAILURE;
		}
		if( setenv("JUDGE_FIFO", config.fifofile, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_FIFO\n", config.judgecode);
			return EXIT_FAILURE;
		}
	}

	if (strlen(config.unixsocket) > 0) {
		if( setenv("JUDGE_UNIX", config.unixsocket, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_UNIX\n", config.judgecode);
			return EXIT_FAILURE;
		}
	}

	if (strlen(config.inetsocket) > 0 && config.inetport > 0) {
		i=0;
		ptr = strtok(config.inetsocket, ", ");
		while(ptr != NULL) {
			sprintf(temp, "JUDGE_INET%d", i++);
			if( setenv(temp, ptr, 1) != 0 ) {
				syslog( LOG_NOTICE, "%s: couldn't set %s\n", config.judgecode, temp);
				return EXIT_FAILURE;
			}
			ptr = strtok(NULL, ", ");
		}
		sprintf(temp,"%d",config.inetport);
		if( setenv("JUDGE_INETPORT", temp, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_INETPORT\n", config.judgecode);
			return EXIT_FAILURE;
		}
	}
*/

	sprintf(temp,"%d",ipc_key);
	if( setenv("JUDGE_IPCKEY", temp, 1) != 0 ) {
		syslog( LOG_NOTICE, "%s: couldn't set JUDGE_IPCKEY\n", config.judgecode);
		return EXIT_FAILURE;
	}

	if((fp_temp = fopen(config.pidfile,"a")) == NULL) {
		syslog( LOG_NOTICE, "%s: can't write to pidfile '%s'. exit.\n", config.judgecode, config.pidfile);
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

	syslog( LOG_NOTICE, "%s: started.\n", config.judgecode);
	while(run) {
		usleep (100000);

/*
 * 
 * check if a child was ended
 * 
 */

		if (run > 1) {
			res = waitpid (-1, NULL, WNOHANG);
			if (res < 0) syslog(LOG_NOTICE, "%s: error while waitpid. errorcode: %d\n", config.judgecode, res);
			else if (res > 0) {
				for (i = 0 ; pid_childs[i] ; i++) {
					if (res == pid_childs[i]) {
						if (i==0)      syslog(LOG_NOTICE, "%s: fifo-child ended (%d)\n", config.judgecode, pid_childs[i]);
						else if (i==1) syslog(LOG_NOTICE, "%s: unix-child ended (%d)\n", config.judgecode, pid_childs[i]);
						else           syslog(LOG_NOTICE, "%s: inet-child ended (%d)\n", config.judgecode, pid_childs[i]);
						pid_childs[i] = 0;
					}
				}
			}
		}

/*
 * 
 * check for FIFO-child
 * 
 */

		if (pid_childs[0] == 0) {
			if ((pid = fork()) < 0) {
				syslog(LOG_NOTICE, "%s: error while fork fifo-child.\n", config.judgecode);
			}

/* Parentprocess */
			else if (pid > 0) {
				pid_childs[0] = pid;
				syslog(LOG_NOTICE, "%s: fifo-child for '%s' forked with PID '%d'.\n", config.judgecode, config.fifofile, pid_childs[0]);
				run++;
			}

/* Childprocess for FIFO */
			else {
				execlp("./judge-fifo", "judge-fifo", config.judgecode, config.fifofile, NULL);
//				sprintf(temp, "judge-fifo %s %s", config.judgecode, config.fifofile);
//				execlp("./judge-env", temp, "/tmp/fifo-env.txt", NULL);
			}
		}

/*
 * 
 * check for UNIX-child
 * 
 */

		if (pid_childs[1] == 0) {

			if ((pid = fork ()) < 0) {
				syslog(LOG_NOTICE, "%s: error while fork unix-child.\n", config.judgecode);
			}

/* Parentprocess */
			else if (pid > 0) {
				pid_childs[1] = pid;
				syslog(LOG_NOTICE, "%s: unix-child for '%s' forked with PID '%d'.\n", config.judgecode, config.unixsocket, pid_childs[1]);
				run++;
			}

/* Childprocess for UNIX */
			else {
				execlp("./judge-sock", "judge-sock", config.judgecode, config.unixsocket, NULL);
//				sprintf(temp, "judge-unix %s %s", config.judgecode, config.unixsocket);
//				execlp("./judge-env", temp, "/tmp/unix-env.txt", NULL);
			}
		}

/*
 * 
 * check for INET-child
 * 
 */

		for (i=2 ; i < 7 ; i++) {
			if (pid_childs[i] == 0) {
				sprintf(temp, "JUDGE_INET%d", i-2);
				if ((pid = fork ()) < 0) {
					syslog(LOG_NOTICE, "%s: error while fork inet-child for '%s'.\n", config.judgecode, getenv(temp));
				}

/* Parentprocess */
				else if (pid > 0) {
					pid_childs[i] = pid;
					syslog(LOG_NOTICE, "%s: inet-child for '%s' forked with PID '%d'.\n", config.judgecode, getenv(temp), pid_childs[i]);
					run++;
				}

/* Childprocess for INET */
				else {
					if( setenv("JUDGE_INET", getenv(temp), 1) != 0 ) {
						syslog( LOG_NOTICE, "%s: couldn't set JUDGE_INET\n", config.judgecode);
					}
					execlp("./judge-sock", "judge-sock", config.judgecode, getenv(temp), NULL);
//					sprintf(temp, "judge-inet %s %s", config.judgecode, getenv(temp));
//					execlp("./judge-env", temp, "/tmp/inet-env.txt", NULL);
				}
			}
		}

/*
 * 
 * check messagequeue
 * 
 */

		msgctl(msgid, IPC_STAT, &msgstat);
//		syslog(LOG_NOTICE, "%s: check messagequeue. %ld\n", config.judgecode, msgstat.msg_qnum);
		if (msgstat.msg_qnum > 0) {
			if ((res = msgrcv(msgid, &msg, MSGLEN, 0, 0)) < 0 ) {
				syslog(LOG_NOTICE, "%s: error %d while reading messagequeue - %d\n", config.judgecode, res, errno);
				return EXIT_FAILURE;
			}
			else {
				syslog(LOG_NOTICE, "%s: incomming command '%s'\n", config.judgecode, msg.text);

// receive 'quit'
				if (strncmp("From quit", msg.text, 9) == 0) {
					run = 0;
					syslog(LOG_NOTICE, "%s: receive 'quit'\n", config.judgecode);
				}

// receive 'atrun'
				if (strncmp("From atrun ", msg.text, 11) == 0) {
					syslog(LOG_NOTICE, "%s: receive 'atrun'\n", config.judgecode);
					sscanf(msg.text + 11,"%ld", &wake);
					tmnow = localtime(&wake);
					tmwake.mon = tmnow->tm_mon;
					tmwake.day = tmnow->tm_mday;
					tmwake.hrs = tmnow->tm_hour;
					tmwake.min = tmnow->tm_min;
					syslog(LOG_NOTICE, "%s: trigger set to '%d.%d. %02d:%02d'.\n", config.judgecode, tmwake.day, tmwake.mon + 1, tmwake.hrs, tmwake.min);
				}
			}
		}

/*
 * 
 * check if dip is free
 * 
 */

		if (semctl (semid, DIP, GETVAL, 0) == UNLOCK) {
			time(&now);
			tmnow = localtime(&now);

// sort masterfile
			if (tmnow->tm_min != 59 && master_sort != 0)
				master_sort = 0;
			if (tmnow->tm_min == 59 && master_sort == 0) {
				semaphore_operation (semid, DIP, LOCK);
				syslog( LOG_NOTICE, "%s: sort dip.master\n", config.judgecode);
				master_mark = 0;
				remove("dip.master.bak");
				rename("dip.master","dip.master.bak");
				if((fp_temp = fopen("dip.master.bak","r")) == NULL) {
					syslog( LOG_NOTICE, "%s: An error occured while opening 'dip.master.bak'.\n", config.judgecode);
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
						syslog( LOG_NOTICE, "%s: An error occured while opening 'dip.master'.\n", config.judgecode);
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
				semaphore_operation (semid, DIP, UNLOCK);
			}

// force timer
			if (now > wake) {
				semaphore_operation (semid, DIP, LOCK);
				sprintf(temp, "%sdip -x", config.judgedir);
				syslog( LOG_NOTICE, "%s: have no trigger, force one. '%s'\n", config.judgecode, temp);
				fp_dip = popen("true ; ./dip -x","w");
				if (fp_dip == NULL) syslog( LOG_NOTICE, "%s: No conection to dip\n", config.judgecode);
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
				semaphore_operation (semid, DIP, UNLOCK);
			}

// trigger timer
			if (tmnow->tm_hour == tmwake.hrs && tmnow->tm_min == tmwake.min && tmnow->tm_mon == tmwake.mon && tmnow->tm_mday == tmwake.day) {
				semaphore_operation (semid, DIP, LOCK);
				sprintf(temp, "%sdip -x", config.judgedir);
				syslog( LOG_NOTICE, "%s: trigger '%s'\n", config.judgecode, temp);
				fp_dip = popen("true ; ./dip -x","w");
				if (fp_dip == NULL) syslog( LOG_NOTICE, "%s: No conection to dip\n", config.judgecode);
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
				semaphore_operation (semid, DIP, UNLOCK);
			}
		}
	}

	cleanup(semid, msgid, &pid_childs[0]);
	return EXIT_SUCCESS;
}
