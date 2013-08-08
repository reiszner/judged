#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#define BUFFER_SIZE 1024



int main (int argc, char **argv) {
	int sockfd, i, n, fd, port;
	struct sockaddr_in adresse;
	struct in_addr inadr;
	struct hostent *host;
	char puffer[BUFFER_SIZE];
	if (argc < 3) {
		printf ("Usage: %s <host> <port> <file> [[file] ...]\n", *argv);
		return EXIT_FAILURE;
	}

	if (inet_aton (argv[1], &inadr))
		host = gethostbyaddr ( (const void*) &inadr, sizeof (inadr), AF_INET );
	else
		host = gethostbyname (argv[1]);

	if (host == NULL) {
		herror ("host not found\n");
		return EXIT_FAILURE;
	}
	adresse.sin_family = AF_INET;
	sscanf(argv[2], "%d", &port);
	adresse.sin_port = htons (port);
	memcpy ( &adresse.sin_addr, host->h_addr_list[0], sizeof (adresse.sin_addr) );
	for (i = 3; i < argc; i++) {
		if ((sockfd = socket (PF_INET, SOCK_STREAM, 0)) < 0) {
			printf("error while socket() ...(%s)\n", strerror(errno));
			exit (EXIT_FAILURE);
		}
		if (connect ( sockfd, (struct sockaddr *) &adresse, sizeof (adresse) ) ) {
			printf("error while connect() ...(%s)\n", strerror(errno));
			exit (EXIT_FAILURE);
		}

		if ((fd = open (argv[i], O_RDONLY)) < 0) {
			printf ("can not open file '%s' (%s)\n", argv[i], strerror(errno));
			continue;
		}

		/* send file to judge */
		while ((n = read (fd, puffer, sizeof (puffer))) > 0) {
			puffer[n]= 0;
			if (write (sockfd, puffer, n) != n) {
				printf("error while write()...(%s)\n", strerror(errno));
				return EXIT_FAILURE;
			}
//			printf("gesendet: %d\n", n);
		}
		if (n < 0) {
			printf ("error while read() ...\n");
			return EXIT_FAILURE;
		}
		close (sockfd);
	}
	return EXIT_SUCCESS;
}
