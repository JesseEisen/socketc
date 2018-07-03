#include "net.h"

/*
 * network can be "tcp", "tcp4", "tcp6", "unix"
 */
static int
parsenetwork(char *network, Sockinfo *si)
{
	if(strcmp(network, "tcp") == 0)
		si->domain = AF_INET;
	else if(strcmp(network, "tcp4") == 0)
		si->domain = AF_INET;
	else if(strcmp(network, "tcp6") == 0)
		si->domain = AF_INET6;
	else if(strcmp(network, "unix") == 0)
		si->domain = AF_UNIX;
	else{
		printf("error network");  // should change to a good practise
		return -1;
	}

	si->type = SOCK_STREAM;

	return 0;

}

static int
sockreuseaddr(int sockfd)
{
	int ret = 0;
	int on = 1;

	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));
	if (ret < 0)
		return -1;

	return 0;
}

static int
parseaddress(char *address, Sockinfo *si)
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
	
	
	if(raw == port)
		ip = strdup("127.0.0.1");
	else{
		*port = '\0';
		ip = strdup(raw);
	}

	port += 1;

	/* like: 127.0.0.1:*/
	if(*port == '\0')
		iport = 0; //auto assigned
	else
		iport = atoi(port);

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

	res = 0;
	strcpy(si->straddr, ip);

BINDERR:
	if(ip != NULL)
		free(ip);

	if(raw != NULL)
		free(raw);
	
	return res;
}

#define BINDWRAP(v) do{ \
	if(bind(fd, (struct sockaddr *)&si.addr.addr##v, sizeof(si.addr.addr##v)) < 0){ \
		fprintf(stderr, "socket bind error\n"); \
		return -1;\
	}\
}while(0)

int
netlisten(char *network, char *address)
{
	int      fd;
	Sockinfo si;

	if(parsenetwork(network, &si) < 0){
		fprintf(stderr, "network parse error\n");
		return -1;
	}

	fd = socket(si.domain, si.type, 0);
	if(fd == -1){
		fprintf(stderr, "open socket error: %s\n", strerror(errno));
		return -1;
	}

	if(parseaddress(address, &si) < 0){
		fprintf(stderr, "address parse error\n");
		return -1;
	}

	if(sockreuseaddr(fd) < 0){
		fprintf(stderr, "socket opt:%s\n", strerror(errno));
		return -1;
	}

	switch(si.domain){
	case AF_INET:
			BINDWRAP(4);
			break;
	case AF_INET6:
			BINDWRAP(6);
			break;
	default:
			fprintf(stderr, "unsupport domain\n");
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

	if(strcmp(network, "tcp") == 0)
		hints.ai_family = AF_UNSPEC;    // use ipv4 or ipv6
	else if(strcmp(network, "tcp4") == 0)
		hints.ai_family = AF_INET;      // ipv4 only
	else if(strcmp(network, "tcp6") == 0)
		hints.ai_family = AF_INET6;     // inpv6 only
	else{
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

	freeaddrinfo(res);

	ret = fd;

NETERR:
	free(raw);
	return ret;
}


#define CONNECTWRAP(v) do{ \
	if(connect(fd, (struct sockaddr *)&si->addr.addr##v, sizeof(si->addr.addr##v)) < 0){ \
		fprintf(stderr, "socket connect error\n"); \
		goto DIALERR; \
	}\
}while(0)


/* client */
Conn *
dial(char *network, char *address)
{
	Conn      *conn = NULL;
	Sockinfo  *si;
	int        fd;

	if((si=malloc(sizeof(Sockinfo))) == NULL){
		fprintf(stderr, "malloc space error\n");
		return NULL;
	}

	if(parsenetwork(network, si) < 0){
		fprintf(stderr, "parse network error\n");
		goto DIALERR;
	}

	fd = socket(si->domain, si->type, 0);
	if(fd < 0){
		fprintf(stderr, "create socket error: %s\n", strerror(errno));
		goto DIALERR;
	}

	if(parseaddress(address, si) < 0){
		fprintf(stderr, "parse address error\n");
		goto DIALERR;
	}

	switch(si->domain){
	case AF_INET:
			CONNECTWRAP(4);
			break;
	case AF_INET6:
			CONNECTWRAP(6);
			break;
	default:
			fprintf(stderr, "unsupport domain\n");
			goto DIALERR;
	}

	conn = malloc(sizeof(Conn));
	if(conn == NULL){
		fprintf(stderr, "malloc error\n");
		goto DIALERR;
	}

	conn->fd = fd;
	conn->si = malloc(sizeof(Sockinfo));
	if(!conn->si){
		fprintf(stderr, "malloc error\n");
		goto DIALERR;
	}

	memcpy(conn->si, si, sizeof(Sockinfo));
	free(si);
	return conn;

DIALERR:
	free(si);
	if(conn != NULL)
		free(conn);
	return NULL;
}
