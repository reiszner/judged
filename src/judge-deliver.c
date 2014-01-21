/***************************************************************************
 *            judge-deliver.c
 *
 *  Die Juli 02 01:33:57 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>



#define BUFFER_SIZE 1024



/*
Aufruf: grep [OPTION]... MUSTER [DATEI]...
Search for PATTERN in each FILE or standard input.
PATTERN is, by default, a basic regular expression (BRE).
Example: grep -i 'hello world' menu.h main.c
*/


int usage (void) {
	printf("Usage: judge-deliver [-h]  <-f fifo | -u unix | -i inet -p port> <-s | -m file>\n");
	printf("Send a message to an Diplomacy-Adjudicator per fifo / unix-domain-socket / inet-socket.\n\n");
	printf("Options:\n");
	printf("    -h          this help\n");
	printf("    -f <fifo>   the fifo to write the message\n");
	printf("    -u <unix>   the unix-domain-socket to write the message\n");
	printf("    -i <inet>   the inet-socket to write the message (-p must also given)\n");
	printf("    -p <port>   the inet-port to write the message\n");
	printf("    -s          the message will be read from stdin\n");
	printf("    -m <file>   the file that contain the message\n");
	return EXIT_FAILURE;
}

int main (int argc, char **argv) {
	int sockfd, port, blocks = 1, cnt, pos;
	struct sockaddr_in inet_address;
	struct sockaddr_un unix_address;
	struct in_addr inaddr;
	struct hostent *host;
	char *msgbuffer;
	FILE *fp;

	char *fvalue = NULL;
	char *uvalue = NULL;
	char *ivalue = NULL;
	char *pvalue = NULL;
	char *mvalue = NULL;
	int sflag = 0, fflag = 0, uflag = 0, iflag = 0, hflag = 0, c;

	while ((c = getopt (argc, argv, "f:u:i:p:m:hs")) != -1)
		switch (c) {

			case 'h':
				hflag = 1;
				break;

			case 's':
				sflag = 1;
				break;

			case 'f':
				fvalue = optarg;
				break;

			case 'u':
				uvalue = optarg;
				break;

			case 'i':
				ivalue = optarg;
				break;

			case 'p':
				pvalue = optarg;
				break;

			case 'm':
				mvalue = optarg;
				break;
/*
			case '?':
				if (optopt == 'f' || optopt == 'u' || optopt == 'i' || optopt == 'p' || optopt == 'm')
					fprintf (stderr, "need parameter for `-%c'.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
				return EXIT_FAILURE;
*/
			default:
				usage();
				return EXIT_FAILURE;

		}

	if (hflag) {
		usage();
		return EXIT_FAILURE;
	}

	if (mvalue == NULL && sflag == 0 && fvalue == NULL && uvalue == NULL && ivalue == NULL && pvalue == NULL) {
		usage();
		return EXIT_FAILURE;
	}

	if (mvalue == NULL && sflag == 0) {
		printf("missing input (option -m or -s)\n");
		return EXIT_FAILURE;
	}

	if(ivalue != NULL && pvalue == NULL) {
		printf("inet-socket needs a port (option -p)\n");
		return EXIT_FAILURE;
	}

		if(ivalue == NULL && pvalue != NULL) {
		printf("inet-socket needs a inet-address (option -i)\n");
		return EXIT_FAILURE;
	}

	if (fvalue != NULL) fflag = 1;
	else if(uvalue != NULL) uflag = 1;
	else if(ivalue != NULL && pvalue != NULL) iflag = 1;

	if (fflag == 0 && uflag == 0 && iflag == 0) {
		printf("missing output (option -f or -u or -i -p)\n");
		return EXIT_FAILURE;
	}

// read the message
	msgbuffer = calloc(sizeof(char), BUFFER_SIZE);
	if (sflag) {
		while ((fgets(&msgbuffer[pos], BUFFER_SIZE, stdin)) != NULL) {
			pos = strlen(msgbuffer);
		}
	} else{
		if((fp = fopen(mvalue, "r")) == NULL) {
			printf ("can't open file '%s' (%s)\n", mvalue, strerror(errno));
			return EXIT_FAILURE;
		}
		while (feof(fp) == 0) {
			if ( (int)(pos / BUFFER_SIZE) == blocks) {
				printf( "pos = %d ; realloc to size %d (%d blocks).\n", pos, BUFFER_SIZE * (blocks + 1), blocks + 1);
				msgbuffer = realloc(msgbuffer, sizeof(char) * BUFFER_SIZE * ++blocks);
			}
			cnt = fread(&msgbuffer[pos * sizeof(char)], sizeof(char), BUFFER_SIZE, fp);
			pos += cnt;
			cnt = 0;
		}
		msgbuffer[pos] = 0;
		clearerr(fp);
		fclose(fp);
	}



// write the message to fifo
	if (fflag) {
		if ((sockfd = open(fvalue, O_WRONLY)) < 0) {
			printf("can't open the fifo '%s' (%s)\n", fvalue, strerror(errno));
			return EXIT_FAILURE;
		}

//		lockf(sockfd, F_LOCK, 0);
/* send message to judge */
		if (!write(sockfd, msgbuffer, strlen(msgbuffer))) {
			printf("can't write to fifo '%s' (%s)\n", fvalue, strerror(errno));
			return EXIT_FAILURE;
		}
//		lockf(sockfd, F_ULOCK, 0);
		close(sockfd);
	}



// write the message to unix-domain-socket
	if (uflag) {
		unix_address.sun_family = AF_LOCAL;
		strcpy(unix_address.sun_path, uvalue);

		if ((sockfd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
			printf("can't open unix-socket '%s' (%s)\n", uvalue, strerror(errno));
			return EXIT_FAILURE;
		}

		if (connect( sockfd, (struct sockaddr *) &unix_address, sizeof (unix_address))) {
			printf("can't connect to unix-socket '%s' (%s)\n", uvalue, strerror(errno));
			return EXIT_FAILURE;
		}

/* send message to judge */
		if (!write(sockfd, msgbuffer, strlen(msgbuffer))) {
			printf("can't write to unix-socket '%s' (%s)\n", uvalue, strerror(errno));
			return EXIT_FAILURE;
		}
		close(sockfd);
	}



// write the message to inet-socket
	if (iflag) {
		if (inet_aton (ivalue, &inaddr))
			host = gethostbyaddr ( (const void*) &inaddr, sizeof (inaddr), AF_INET );
		else
			host = gethostbyname (ivalue);

		if (host == NULL) {
			herror("host not found. exit.\n");
			return EXIT_FAILURE;
		}
		inet_address.sin_family = AF_INET;
		sscanf(pvalue, "%d", &port);
		inet_address.sin_port = htons (port);
		memcpy ( &inet_address.sin_addr, host->h_addr_list[0], sizeof (inet_address.sin_addr) );

		if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
			printf("can't open inet-socket '%s' (%s)\n", ivalue, strerror(errno));
			return EXIT_FAILURE;
		}

		if (connect( sockfd, (struct sockaddr *) &inet_address, sizeof (inet_address) ) ) {
			printf("can't connect to inet-socket '%s' (%s)\n", ivalue, strerror(errno));
			return EXIT_FAILURE;
		}

/* send message to judge */
		if (!write(sockfd, msgbuffer, strlen(msgbuffer))) {
			printf("can't write to inet-socket '%s' (%s)\n", ivalue, strerror(errno));
			return EXIT_FAILURE;
		}
		close(sockfd);
	}

	free(msgbuffer);
	msgbuffer = NULL;
	return EXIT_SUCCESS;
}
