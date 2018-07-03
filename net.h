#ifndef _NET_H_
#define _NET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXBACKLOG  128

typedef struct Sockinfo Sockinfo;
struct Sockinfo
{
	int domain;
	int type;
	union
	{
		struct sockaddr_in  addr4;
		struct sockaddr_in6 addr6;
	}addr;
	char straddr[INET6_ADDRSTRLEN];
};

typedef struct Conn Conn;
struct Conn
{
	int     	 fd;
	Sockinfo 	*si;
};

extern int netlisten(char *, char *);
extern int netlistenb(char *, char *);
extern Conn *dial(char *, char *);

#endif

