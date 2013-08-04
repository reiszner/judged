/***************************************************************************
 *            fifo.c
 *
 *  Die Juli 02 01:50:59 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "fifo.h"

int create_fifo(char *fifoname)
{
	int ret;
	if((ret = mkfifo(fifoname, 0666)) == -1) {
		if(errno == EEXIST) {
			perror("fifo exists, use it");
			ret = 0;
		}
		else {
			perror("An error occured while make the fifo");
		}
	}
	return ret;
}



int open_fifo(char *fifoname)
{
	int fd;
	if ((fd = open(fifoname, O_RDONLY)) == -1) {
		perror ("An error occured while open the fifo");
		return 0;
	}
	return fd;
}



int close_fifo(int fd)
{
	close(fd);
	return 0;
}



int remove_fifo(char *fifoname)
{
	remove(fifoname);
	return 0;
}
