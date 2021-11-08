#include "stems.h"

void getargs_pc(char* hostname, int* port, char* filename, char* name, float* threshold)
{
	FILE* fp;

	fp = fopen("config-pc.txt", "r"); //config-pc에서 임계값을 읽어온다.
	if (fp == NULL)
		unix_error("config-pc.txt file does not open.\n");


	fscanf(fp, "%s", hostname);
	fscanf(fp, "%d", port);
	fscanf(fp, "%s", filename);
	fscanf(fp, "%s", name);
	fscanf(fp, "%f", threshold);
	fclose(fp);
}


void parseData(char* data, char* sensorname, char* time, float* sensorValue) {
	/*받아온 값을 이름, 시간, 값으로 구분하는 함수*/
	strtok(data, "="); //이름 구분
	sprintf(sensorname, "%s", strtok(NULL, "&"));

	strtok(NULL, "=");  //시간 구분
	sprintf(time, "%s", strtok(NULL, "&"));

	strtok(NULL, "=");  //값 구분
	*sensorValue = atof(strtok(NULL, "&"));
}

void clientSend(int fd, char* filename, char* body, int port)
{
	char buf[MAXLINE];
	char hostname[MAXLINE];

	Gethostname(hostname, MAXLINE);

	sprintf(buf, "POST %s HTTP/1.1\r\n", filename);
	sprintf(buf, "%sHost: %s:%d\r\n", buf, hostname, port);
	sprintf(buf, "%sConnection: keep-alive\r\n", buf);
	sprintf(buf, "%sContent-Length: %d\r\n", buf, (int)strlen(body));
	sprintf(buf, "%s\r\n", buf);
	sprintf(buf, "%s%s", buf, body);
	Rio_writen(fd, buf, strlen(buf));
}

void clientPrint(int fd)
{
	rio_t rio;
	char buf[MAXBUF];
	int length = 0;
	int n;

	Rio_readinitb(&rio, fd);

	/* Read and display the HTTP Header */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (strcmp(buf, "\r\n") && (n > 0)) {
		n = Rio_readlineb(&rio, buf, MAXBUF);

		/* If you want to look for certain HTTP tags... */
		if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
		}
	}

	/* Read and display the HTTP Body */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (n > 0) {
		printf("%s", buf);
		n = Rio_readlineb(&rio, buf, MAXBUF);
	}
}

void userTask(char* hostname, int port, char* filename, char* body)
{
	int clientfd;
	clientfd = Open_clientfd(hostname, port);
	clientSend(clientfd, filename, body, port);
	clientPrint(clientfd);
	Close(clientfd);
}

int main(void) {
	char hostname[MAXLINE], filename[MAXLINE], name[MAXLINE];
	int port;
	float threshold;//임계값

	int pipe;

	getargs_pc(hostname, &port, filename, name, &threshold);

	while (1) {
		while ((pipe = open(FIFO, O_RDWR)) == -1) {} //FIFO 개방

		sleep(1);

		int len, nread;
		char data[MAXLINE];

		while ((nread = Read(pipe, &len, sizeof(int))) < 0) {} //값의 길이를 먼저 읽는다
		while ((nread = Read(pipe, data, len)) < 0) {} //값을 읽어온다
		Close(pipe);
		unlink(FIFO);  //FIFO 제거

		char sensorname[MAXLINE]; //센서 이름
		char time[MAXLINE];  //시간
		char sendData[MAXLINE]; //alarm으로 보낼 전체 값
		float sensorValue; //센서 값

		strcpy(sendData, data);  //받은 값을 sendData로 복사함
		parseData(data, sensorname, time, &sensorValue);  //값을 구분
		if (strcmp(name, sensorname) == 0) { //config-pc에 적힌 이름과 센서명이 같다면
			if (sensorValue > threshold) { //센서 값이 임계값보다 크다면
				userTask(hostname, port, filename, sendData); //전체 값을 보내준다.
			}
		}
	}
	return 0;
}
