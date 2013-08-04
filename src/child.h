/***************************************************************************
 *            child.h
 *
 *  Mon Juli 08 21:02:48 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

pid_t add_child(pid_t pid, pid_t childs_pid[], int);
pid_t del_child(pid_t childs_pid[], int);
