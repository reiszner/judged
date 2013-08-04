/***************************************************************************
 *            socket.c
 *
 *  Die Juli 02 01:59:28 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "socket.h"

int create_socket( int af, int type, int protocol ) {
	int sock;
	const int y = 1;
	sock = socket(af, type, protocol);
	if (sock >= 0 && af != AF_LOCAL) setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));
	return sock;
}



int bind_in_socket(int *sock, char *address, unsigned short port) {
	struct sockaddr_in server;
	struct in_addr addr;
	struct hostent *hostinfo;

	memset( &server, 0, sizeof (server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	if (strncmp("0.0.0.0",address,7) == 0) {
		server.sin_addr.s_addr = INADDR_ANY;
	}
	else {
		if (inet_pton (AF_INET, address, &addr))
			hostinfo = gethostbyaddr( (const void*) &addr, sizeof(addr), AF_INET);
		else
			hostinfo = gethostbyname(address);
		if (hostinfo == NULL) {
			return -2;
		}
		memcpy(&server.sin_addr, hostinfo->h_addr_list[0], sizeof(server.sin_addr));
	}

	return ( bind( *sock, (struct sockaddr *) &server, sizeof(server)));
}



int bind_un_socket(int *sock, char *unixsocket) {
	struct sockaddr_un server;
	memset( &server, 0, sizeof (server));
	server.sun_family = AF_LOCAL;
	strcpy(server.sun_path, unixsocket);
	return (bind( *sock, (struct sockaddr *)&server, sizeof(server)));
}



int listen_socket( int *sock ) {
	return (listen(*sock, 100));
}



int accept_socket(int *sock) {
	struct sockaddr_in client;
	socklen_t len;
	int new_sock; 
	len = sizeof(client);
	new_sock = accept( *sock,(struct sockaddr *)&client, &len );
	return new_sock;
}



void TCP_recv( int *sock, char *data, size_t size) {
	int len;
	len = recv (*sock, data, size, 0);
	if( len > 0 || len != -1 ) data[len] = '\0';
}



void close_socket( int *sock ) {
	close(*sock);
}
