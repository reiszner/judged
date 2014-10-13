/***************************************************************************
 *            misc.h
 *
 *  Die Juli 02 01:33:57 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <syslog.h>

#define FREQUENCY 100000

typedef void (sigfunk) (int);



sigfunk *signal (int, sigfunk);
void strtolower (char *);
int chowngrp(int, int);
void logging(const int, const wchar_t *, ...);
