/***************************************************************************
 *            ipc.h
 *
 *  Die Juli 02 01:15:37 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <wchar.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <sys/shm.h>

#define LOCK -1
#define UNLOCK 1
#define MSGLEN 1024
#define BUFFER_SIZE 1024
#define QRESERVE 30

struct message {
	long prio;
	wchar_t text[MSGLEN];
};

int init_semaphore (key_t);
int get_semaphore (key_t);
int semaphore_operation (int, unsigned short, int);
int init_msgqueue (key_t);
int get_msgqueue (key_t);
int send_msg (int, int, int, wchar_t *, ...);
int init_sharedmemory (key_t);
int get_sharedmemory (key_t);
int send_sharedmemory(void *, char *, int, int, int);
char *resive_sharedmemory(void *, int, int, int);
