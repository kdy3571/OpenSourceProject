#include "stems.h"
#include "request.h"

void getargs_ps(int *port)
{
	/*config-ps 파일 읽어옴*/
	FILE *fp;

	if ((fp = fopen("config-ps.txt", "r")) == NULL)
		unix_error("config-ps.txt file does not open.");

	fscanf(fp, "%d", port);
	fclose(fp);
}

void consumer(int connfd, long arrivalTime)
{
	requestHandle(connfd, arrivalTime);
	Close(connfd);
}

int main(void)
{
	/*server.c와 크게 다른게 없음*/
	int listenfd, connfd, port, clientlen;
	struct sockaddr_in clientaddr;
	
	initWatch();
	getargs_ps(&port);
	
	listenfd = Open_listenfd(port);
	while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *)&clientlen);
		consumer(connfd, getWatch());
	}
	return(0);
}
