/***************************************************************************
 *            fifo.h
 *
 *  Die Juli 02 01:50:59 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int create_fifo(char *);
int open_fifo(char *);
int close_fifo(int);
int remove_fifo(char *);
