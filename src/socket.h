/***************************************************************************
 *            socket.h
 *
 *  Die Juli 02 01:59:28 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>

int create_socket(int, int, int);
int bind_in_socket(int *, char *, unsigned short);
int bind_un_socket(int *, char *);
int listen_socket(int *);
int accept_socket(int *);
void TCP_recv( int *, char *, size_t);
void close_socket( int *);
