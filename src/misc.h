/***************************************************************************
 *            misc.h
 *
 *  Die Juli 02 01:33:57 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <ctype.h>
#include <signal.h>
#include <unistd.h>

typedef void (sigfunk) (int);

sigfunk *signal (int, sigfunk);
void strtolower (char *);
int chowngrp(int, int);
