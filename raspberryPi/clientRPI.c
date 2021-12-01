#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/time.h>
#include <time.h>
#include "stems.h"
#include <ctype.h>
#include <wiringPi.h>

#define MAXTIMINGS 83
#define DHTPIN 7
#define BTN 9

void getargs_pi(char* hostname, int* port, char* filename, float* period)
{
	FILE* fp;

	fp = fopen("config-pi.txt", "r");
	if (fp == NULL)
		unix_error("config-pi.txt file does not open.");

	fscanf(fp, "%s", hostname);
	fscanf(fp, "%d", port);
	fscanf(fp, "%s", filename);
	fscanf(fp, "%f", period);
	fclose(fp);
}

void clientSend(int fd, char* filename, char* body)
{
	char buf[MAXLINE];
	char hostname[MAXLINE];

	Gethostname(hostname, MAXLINE);

	/* Form and send the HTTP request */
	sprintf(buf, "POST %s HTTP/1.1\n", filename);
	sprintf(buf, "%sHost: %s\n", buf, hostname);
	sprintf(buf, "%sContent-Type: text/plain; charset=utf-8\n", buf);
	sprintf(buf, "%sContent-Length: %d\n\r\n", buf, strlen(body));
	sprintf(buf, "%s%s\n", buf, body);
	Rio_writen(fd, buf, strlen(buf));
	printf("..Sending %s ...\n", body);
}

/*
 * Read the HTTP response and print it out
 */
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
		/* If you want to look for certain HTTP tags...
		if (sscanf(buf, "Content-Length: %d ", &length) == 1)
		  printf("Length = %d\n", length);
		printf("Header: %s", buf);*/
		n = Rio_readlineb(&rio, buf, MAXBUF);
	}

	/* Read and display the HTTP Body */
	n = Rio_readlineb(&rio, buf, MAXBUF);
	while (n > 0) {
		//printf("%s", buf);
		n = Rio_readlineb(&rio, buf, MAXBUF);
	}
}

/* currently, there is no loop. I will add loop later */
void userTask(char* myname, char* hostname, int port, char* filename, long int time, float value)
{
	int clientfd;
	char msg[MAXLINE];

	sprintf(msg, "name=%s&time=%ld&value=%.1f", myname, time, value);
	clientfd = Open_clientfd(hostname, port);
	clientSend(clientfd, filename, msg);
	clientPrint(clientfd);
	Close(clientfd);
}

int dht11_dat[5] = { 0, 0, 0, 0, 0 }; 

void read_dht11_dat(char* hostname, int port, char* filename, float period)
{
	uint8_t laststate = HIGH;
	uint8_t counter = 0;
	uint8_t j = 0, i;
	uint8_t flag = HIGH;
	uint8_t state = 0;
	float f;
	long int t;

	memset(dht11_dat, 0, sizeof(int) * 5);
	pinMode(DHTPIN, OUTPUT);
	digitalWrite(DHTPIN, LOW);
	delay(18);
	digitalWrite(DHTPIN, HIGH);
	delayMicroseconds(30);
	pinMode(DHTPIN, INPUT);

	for (i = 0; i < MAXTIMINGS; i++) {
		counter = 0;
		while (digitalRead(DHTPIN) == laststate) {
			counter++;
			delayMicroseconds(1);
			if (counter == 200) break;
		} // while
		laststate = digitalRead(DHTPIN);
		if (counter == 200) break;
		if ((i >= 4) && (i % 2 == 0)) {
			dht11_dat[j / 8] <<= 1;
			if (counter > 40)
				dht11_dat[j / 8] |= 1; j++;
		} // if 
	} // for
	if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xff))) {
		char v_ch[MAXLINE];
		float value;
		t = time(NULL);
		delay(period * 1000 / 2);
		sprintf(v_ch, "%d.%d", dht11_dat[2], dht11_dat[3]);
		value = atof(v_ch);
		userTask("temperature", hostname, port, filename, t, value);
		delay(period * 1000 / 2);
		sprintf(v_ch, "%d.%d", dht11_dat[0], dht11_dat[1]);
		value = atof(v_ch);
		userTask("humidity", hostname, port, filename, t, value);
	}
	else
		printf("Data get failed\n");
}

int main(void) 
{
	char hostname[MAXLINE], filename[MAXLINE];
	int port;
	float period;
 
	getargs_pi(hostname, &port, filename, &period);
	
	printf("dht11 Raspberry pi\n"); 
	if (wiringPiSetup() == -1)
		exit(1); 

	pinMode(BTN, INPUT);

	while (1) {
		if (digitalRead(BTN) == HIGH) {
			read_dht11_dat(hostname, port, filename, period);
			delay(1000);
		}
		else {
			long int t;
			t = time(NULL);
			userTask("clear", hostname, port, filename, t, CLEAR);
			printf("DATABASE CELAR\n");
			return(0);
		}
	}
}
