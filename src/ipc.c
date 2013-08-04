/***************************************************************************
 *            ipc.c
 *
 *  Die Juli 02 01:15:37 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "ipc.h"

int init_semaphore (key_t key) {
/* Testen, ob das Semaphor bereits existiert */
	int semid;
	semid = semget (key, 0, IPC_PRIVATE);
	if (semid < 0) {
		/* ... existiert noch nicht, also anlegen       */
		/* Alle Zugriffsrechte der Dateikreierungsmaske */
		/* erlauben                                     */
		umask(0);
		semid = semget (key, 2, IPC_CREAT | IPC_EXCL | 0666);
		if (semid < 0) {
			return -1;
		}
		/* Semaphor mit 1 initialisieren */
		if (semctl (semid, FIFO, SETVAL, (int) 1) == -1) return -1;
		if (semctl (semid, DIP, SETVAL, (int) 1) == -1) return -1;
	}
	return semid;
}



int init_msgqueue (key_t key) {
	int msgid;
	msgid = msgget (key, IPC_PRIVATE);
	if (msgid < 0) {
		umask(0);
		msgid = msgget (key, IPC_CREAT | IPC_EXCL | 0666);
		if (msgid < 0) {
			return -1;
		}
	}
	return msgid;
}



int semaphore_operation (int semid, unsigned short num , int op) {
	struct sembuf semaphore;
	semaphore.sem_num = num;
	semaphore.sem_op = op;
	semaphore.sem_flg = SEM_UNDO;
	if( semop (semid, &semaphore, 1) == -1) {
		perror(" semop ");
		exit (EXIT_FAILURE);
	}
	return 1;
}
