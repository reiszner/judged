/***************************************************************************
 *            ipc.c
 *
 *  Die Juli 02 01:15:37 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "ipc.h"
#include "misc.h"

int init_semaphore (key_t key) {
	int semid;
	semid = semget (key, 0, IPC_PRIVATE);
	if (semid < 0) {
		umask(0);
		semid = semget (key, 1, IPC_CREAT | IPC_EXCL | 0600);
		if (semid < 0) return -1;
	}
	return semid;
}



int get_semaphore (key_t key) {
	int semid;
	semid = semget (key, 0, IPC_PRIVATE);
	return semid;
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



int init_msgqueue (key_t key) {
	int msgid;
	msgid = msgget (key, IPC_PRIVATE);
	if (msgid < 0) {
		umask(0);
		msgid = msgget (key, IPC_CREAT | IPC_EXCL | 0600);
		if (msgid < 0) return -1;
	}
	return msgid;
}



int get_msgqueue (key_t key) {
	int msgid;
	msgid = msgget (key, IPC_PRIVATE);
	return msgid;
}



int send_msg (int id, int mode, int partner, wchar_t *format, ...) {

	struct message msg;
	struct msqid_ds msgstat;
	va_list arglist;

	va_start(arglist,format);
	msg.prio = partner;
	vswprintf(msg.text, MSGLEN, format, arglist);

	if (mode == IPC_NOWAIT) {
		msgsnd(id, &msg, MSGLEN, mode);
//		wprintf (L"SEND -- free: %d / self: %d / partner: %d\n", 16 - msgstat.msg_qnum, getpid(), partner);
	}
/*
ushort msg_cbytes;    // Anzahl belegten Bytes in Queue
ushort msg_qnum;      // Anzahl der Nachrichten in Queue
ushort msg_qbytes;    // Max. Anzahl Bytes der Queue
*/
	else {
		while (1) {
			msgctl(id, IPC_STAT, &msgstat);
			if ((((msgstat.msg_qbytes - msgstat.msg_cbytes) * 100) / msgstat.msg_qbytes) > QRESERVE) {
//				wprintf (L"SEND -- free: %d / self: %d / partner: %d\n", 16 - msgstat.msg_qnum, getpid(), partner);
				msgsnd(id, &msg, MSGLEN, mode);
				break;
			}
//			else wprintf(L"HOLD -- free: %d / self: %d / partner: %d\n", 16 - msgstat.msg_qnum, getpid(), partner);
			usleep (FREQUENCY);
		}
	}
	return 0;
}



int init_sharedmemory (key_t key) {
	int shmid;
	shmid = shmget (key, 0, IPC_PRIVATE);
	if (shmid < 0) {
		umask(0);
		shmid = shmget (key, BUFFER_SIZE, IPC_CREAT | SHM_R | SHM_W | IPC_EXCL | 0600);
		if (shmid < 0) return -1;
	}
	return shmid;
}



int get_sharedmemory (key_t key) {
	int shmid;
	shmid = shmget (key, 0, IPC_PRIVATE);
	return shmid;
}




int send_sharedmemory(void *shmdata, char *msgbuffer, int msgid, int own_pid, int partner) {

	struct message msg;
	int pos = 0, blocks;
	blocks = strlen(msgbuffer) / BUFFER_SIZE + 1;

	swprintf( msg.text, MSGLEN, L"SHM_DATA");
	msg.prio=partner;
	msgsnd(msgid, &msg, MSGLEN, 0);

	while (blocks > 0) {
		msgrcv(msgid, &msg, MSGLEN, own_pid, 0);

		if (wcsncmp(L"SHM_EMPTY", msg.text, 9) == 0) {
			memcpy(shmdata, msgbuffer+(BUFFER_SIZE * pos++), BUFFER_SIZE);
			if (pos == blocks) swprintf( msg.text, MSGLEN, L"SHM_LAST");
			else swprintf( msg.text, MSGLEN, L"SHM_FULL");
			msg.prio=partner;
			msgsnd(msgid, &msg, MSGLEN, 0);
		}

		else if (wcsncmp(L"SHM_READY", msg.text, 9) == 0) {
			blocks = 0;
		}

		else {
			blocks = -1;
		}

	}
	return blocks;
}



char *resive_sharedmemory(void *shmdata, int msgid, int own_pid, int partner) {

	struct message msg;
	int pos = 1;
	char *msgbuffer;

	msgbuffer = calloc(BUFFER_SIZE * pos, sizeof(char));

	msg.prio=partner;
	swprintf(msg.text, MSGLEN, L"SHM_EMPTY");
	msgsnd(msgid, &msg, MSGLEN, 0);

	while (pos) {
		msgrcv(msgid, &msg, MSGLEN, own_pid, 0);

		if (wcsncmp(L"SHM_DATA", msg.text, 10) == 0) {
			msg.prio=partner;
			swprintf(msg.text, MSGLEN, L"SHM_EMPTY");
			msgsnd(msgid, &msg, MSGLEN, 0);
		}

		if (wcsncmp(L"SHM_FULL", msg.text, 8) == 0) {
			memcpy(msgbuffer+(BUFFER_SIZE * (pos - 1)), shmdata, BUFFER_SIZE);
			msgbuffer = realloc(msgbuffer, sizeof(char) * BUFFER_SIZE * ++pos);
			msg.prio=partner;
			swprintf(msg.text, MSGLEN, L"SHM_EMPTY");
			msgsnd(msgid, &msg, MSGLEN, 0);
		}

		else if (wcsncmp(L"SHM_LAST", msg.text, 8) == 0) {
			memcpy(msgbuffer+(BUFFER_SIZE * (pos - 1)), shmdata, BUFFER_SIZE);
			msg.prio=partner;
			swprintf(msg.text, MSGLEN, L"SHM_READY");
			msgsnd(msgid, &msg, MSGLEN, 0);
			pos = 0;
		}

		else {
			free(msgbuffer);
			msgbuffer = NULL;
			pos = 0;
		}
	}
	return msgbuffer;
}
