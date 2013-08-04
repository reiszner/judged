/***************************************************************************
 *            config.h
 *
 *  Die Juli 02 01:25:11 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

#include "misc.h"

struct Config {
	char file[1024];
	char judgeuser[128];
	int  judgeuid;
	char judgegroup[128];
	int  judgegid;
	char judgedir[1024];
	char judgecode[8];
	char ourselves[128];
	char judgekeeper[128];
	char gateway[128];
	char pidfile[128];
	char unixsocket[128];
	char inetsocket[128];
	int  inetport;
	char fifofile[128];
	int  fifochilds;
	int  loginput;
	int  logoutput;
} config;

struct Config *read_config(struct Config *);
