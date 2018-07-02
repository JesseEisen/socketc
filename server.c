#include "net.h"

typedef struct sockinfo sockinfo;
struct sockinfo
{
	int domain;
	int type;
	int protocol;
	union
	{
		struct sockaddr_in  addr4;
		struct sockaddr_in6 addr6;
	}addr;
};

/*
 * network can be "tcp", "tcp4", "tcp6", "unix"
 */
static int
sockcreate(char *network, sockinfo *si)
{
	if(strcmp(network, "tcp") == 0){
		si->domain = AF_INET;
	}else if(strcmp(network, "tcp4") == 0){
		si->domain = AF_INET;
	}else if(strcmp(network, "tcp6") == 0){
		si->domain = AF_INET6;
	}else if(strcmp(network, "unix") == 0){
		si->domain = AF_UNIX;
	}else{
		printf("error network");  // should change to a good practise
		return -1;
	}

	si->type = SOCK_STREAM;

	return socket(si->domain, si->type, 0);
}

static int
sockreuseaddr(int sockfd)
{
	int ret = 0;
	int on = 1;

	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));
	if (ret < 0) {
		return -1;
	}

	return 0;
}

static int
sockbind(int fd, char *address, sockinfo *si)
{
	int   iport, res = -1;
	char *port = NULL;
	char *ip   = NULL;
	char *raw  = strdup(address);

	if(raw == NULL){
		fprintf(stderr, "strdup error %s\n", strerror(errno));
		return -1;
	}
	
	port = strchr(raw, ':');
	if(port == NULL){
		printf("invalid address");
		goto BINDERR;
	}
	
	if(raw == port){
		ip = strdup("127.0.0.1");
	}else{
		ip = raw;
	}

	*port++ = '\0';

	/* like: 127.0.0.1:*/
	if(*port == '\0'){
		iport = 0; //auto assigned
	}else{
		iport = atoi(port);
	}

	if(iport < 10 || iport > 65535){
		printf("invalid port number\n");
		goto BINDERR;
	}

	switch(si->domain){
	case AF_INET:
				si->addr.addr4.sin_family = AF_INET; 
				si->addr.addr4.sin_port   = htons(iport); 
				if(inet_pton(AF_INET, ip, &(si->addr.addr4.sin_addr)) <= 0){ 
					printf("invalid ipv4 address\n"); 
					goto BINDERR;
				}
				break;
	case AF_INET6:
				si->addr.addr6.sin6_family = AF_INET6; 
				si->addr.addr6.sin6_port   = htons(iport); 
				if(inet_pton(AF_INET6, ip, &(si->addr.addr6.sin6_addr)) <= 0){ 
					printf("invalid ipv6 address\n"); 
					goto BINDERR;
				}
				break;
	case AF_UNIX:
				//donothing
				break;
	default:
				printf("invalid domain\n");
				goto BINDERR;
	}

	if(sockreuseaddr(fd) < 0){
		fprintf(stderr, "socket opt:%s\n", strerror(errno));
		goto BINDERR;
	}

	if(bind(fd, (struct sockaddr *)&si->addr.addr4, sizeof(si->addr.addr4)) < 0){
		printf("socket bind error\n");
		goto BINDERR;
	}
	
	res = 0;

BINDERR:
	if(ip != NULL)
		free(ip);
	free(raw);
	return res;
}


int
netlisten(char *network, char *address)
{
	int      fd;
	sockinfo si;

	fd = sockcreate(network, &si);
	if(fd == -1){
		fprintf(stderr, "open socket error: %s\n", strerror(errno));
		return -1;
	}

	if(sockbind(fd, address, &si) < 0){
		fprintf(stderr, "bind socket error: %s\n", strerror(errno));
		return -1;
	}

	if(listen(fd, MAXBACKLOG) < 0){
		fprintf(stderr, "listen error: %s\n", strerror(errno));
		return -1;
	}
	
	return fd;
}

/*
 * use getaddrinfo to lookup address, this will cause block.
 */
int 
netlistenb(char *network, char *address)
{
	int fd, status, ret = -1;
	char *raw, *port;
	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof hints);

	if(strcmp(network, "tcp") == 0){
		hints.ai_family = AF_UNSPEC;    // use ipv4 or ipv6
	}else if(strcmp(network, "tcp4") == 0){
		hints.ai_family = AF_INET;      // ipv4 only
	}else if(strcmp(network, "tcp6") == 0){
		hints.ai_family = AF_INET6;     // inpv6 only
	}else{
		fprintf(stderr, "invalid network format\n");
		return -1;
	}

	hints.ai_socktype = SOCK_STREAM;  // tcp
	hints.ai_flags    = AI_PASSIVE;   // use local ip to bind

	raw = strdup(address);
	if(raw == NULL){
		fprintf(stderr, "strdup error\n");
		goto NETERR;
	}

	port = strchr(raw, ':');
	if(port == NULL){
		fprintf(stderr, "invalid address");
		goto NETERR;
	}

	if(raw == port){
		*port++ = '\0';
		 raw    = NULL;
	}

	if((status=getaddrinfo(raw, port, &hints, &res)) != 0){
		fprintf(stderr, "cannot get addr info: %s\n", gai_strerror(status));
		goto NETERR;
	}
	
	if((fd=socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
		fprintf(stderr, "create socket error: %s\n",strerror(errno));
		goto NETERR;
	}
	
	if(sockreuseaddr(fd) < 0){
		fprintf(stderr, "socket opt:%s\n", strerror(errno));
		goto NETERR;
	}

	if(bind(fd, res->ai_addr, res->ai_addrlen) < 0){
		fprintf(stderr, "bind socket error: %s\n", strerror(errno));
		goto NETERR;
	}

	if(listen(fd, MAXBACKLOG) < 0){
		fprintf(stderr, "listen socket error:%s\n", strerror(errno));
		goto NETERR;
	}

	ret = fd;
NETERR:
	free(raw);
	return ret;
}
