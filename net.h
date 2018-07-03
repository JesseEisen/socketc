#ifndef _NET_H_
#define _NET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define MAXBACKLOG  	128
#define READ_TIMEOUT 	1
#define READ_RETRY_MAX  1
#define WRITE_TIMEOUT   1

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

enum Retval{
	CONN_CLOSE = -2
} ;


extern int netlisten(char *, char *);
extern int netlistenb(char *, char *);
extern Conn *dial(char *, char *);
extern int netread(int, void *, int);
extern int netwrite(int, void *, int);
extern int closerd(int);
extern int closewr(int);
extern Conn *netaccept(int);
extern Conn *netacceptnb(int);

#endif

