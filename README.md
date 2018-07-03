## socket in c

Use this lib to wrap socket function. And make create a socket in C more easy.

## Usage

Here is a simple code to create a tcp client.

```c
#include "net.h"

#define BUFFLEN  1024

int main(void)
{
	Conn *conn;
	char buf[BUFFLEN];

	conn = dial("tcp", ":9000");
	if(conn == NULL)
		return -1;

	if(netread(conn->fd, buf, BUFFLEN) < 0)
		return -1;

	if(netwrite(conn->fd, buf, BUFFLEN) < 0)
		return -1;
	
	close(fd);
	free(conn->si);
	free(conn);

	return 0;
}
```

And this is a tcp server.

```c
#include "net.h"

#define BUFFLEN 1024

int main(void)
{
	int   fd;
	Conn *conn;
	char  buf[BUFFLEN] = "Hello World";

	fd = netlisten("tcp", ":9000");
	if(fd < 0)
		return -1;
	
	if((conn=netaccpet(fd)) == NULL)
		return -1;

	if(netwrite(conn->fd, buf, BUFFLEN) < 0)
		return -1;

	memset(buf, 0, BUFFLEN);
	if(netread(conn->fd, buf, BUFFLEN) < 0)
		return -1;

	printf("[server]: recv %s\n", buf);
	close(fd);
	free(conn->si);
	free(conn);

	return 0;
}
```

## Other
This code is not stable, and need more test. And the data structure may also be changed. So use it carefully.

