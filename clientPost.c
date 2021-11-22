/*
 * clientPost.c: A very, very primitive HTTP client for sensor
 *
 * To run, prepare config-cp.txt and try:
 *      ./clientPost
 *
 * Sends one HTTP request to the specified HTTP server.
 * Get the HTTP response.
 */

 /*sprintf(query, "UPDATE sensorList set cnt = cnt + 1 WHERE name = '%s'", name);
     mysql_query(conn, query);
     sprintf(query, "UPDATE sensorList L, sensorCal C SET L.ave = C.avg WHERE L.name = C.name");
     mysql_query(conn, query);
     sprintf(query, "UPDATE sensorList L JOIN sensorCal C ON L.max < C.value SET L.max = C.value WHERE L.Name = C.name");
     mysql_query(conn, query);*/
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

char myname[MAXLINE], hostname[MAXLINE], filename[MAXLINE];
int port;
float value;    // thread 사용을 위해 전역 변수화

void* producer(void* arg);

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

void getargs_cp()
{
    FILE* fp;

    fp = fopen("config-cp.txt", "r");
    if (fp == NULL)
        unix_error("config-cp.txt file does not open.");

    fscanf(fp, "%s", &myname);
    fscanf(fp, "%s", &hostname);
    fscanf(fp, "%d", &port);
    fscanf(fp, "%s", &filename);
    fscanf(fp, "%f", &value);
    fclose(fp);
}

void command_shell()
{
    char* tok;
    int random;
    long int t;
    srand((unsigned)time(NULL));

    printf("If you want to see commands, type 'help'\n");
    while (1) {
        char str[MAXLINE], command[MAXLINE], check[MAXLINE] = { NULL };

        //printf(">>\n");
        scanf("%[^\n]s%", str);
        getchar();

        tok = strtok(str, " ");
        strcpy(command, tok);

        int tok_num = 0;
        while (tok != NULL) {
            tok = strtok(NULL, " ");
            if (tok_num == 0 && tok != NULL)
                strcpy(check, tok);
            tok_num++;
        }

        if (tok_num <= 2) {
            if (!strcmp(command, "help")) {
                if (!*check) {
                    printf("help: list avaliable commands.\n");
                    printf("name: print current seonsor name.\n");
                    printf("name <sensor>: change sensor name to <sensor>.\n");
                    printf("value: print current value of sensor.\n");
                    printf("value <n>: set sensor value to <n>.\n");
                    printf("send: send (current sensor name, time, value) to server.\n");
                    printf("random <n>:  send (current sensor name, time, value) to server <n> times.\n");
                    printf("prand <n>:  send (current sensor name, time, value) to server simultaneously <n> times.\n");
                    printf("quit: quit the program.\n");
                }
                else
                    printf("help: no help topics match '%s'\n", check);
            }

            else if (!strcmp(command, "name")) {
                if (*check) {
                    strcpy(myname, check);
                    printf("Sensor name is changed to '%s'\n", myname);
                }
                else
                    printf("Current sensor is '%s'\n", myname);
            }

            else if (!strcmp(command, "value")) {
                if (*check) {
                    if (atof(check)) {
                        value = atof(check);
                        printf("Sensor value is changed to %.1f\n", value);
                    }
                    else
                        if (isdigit(check[0])) {
                            value = atof(check);
                            printf("Sensor value is changed to %.1f\n", value);
                        }
                        else
                            printf("%s: value <n> is wrong\n", check);
                }
                else
                    printf("Current sensor is %.1f\n", value);
            }

            else if (!strcmp(command, "send")) {
                if (!*check) {
                    t = time(NULL);
                    userTask(myname, hostname, port, filename, t, value);
                }
                else
                    printf("send: no send topics match '%s'\n", check);
            }

            else if (!strcmp(command, "random")) {
                if (*check) {
                    if (atoi(check) > 0) {
                        random = atoi(check);
                        float temp = value;
                        for (int i = 0; i < random; i++) {
                            value = temp;
                            t = time(NULL);
                            if (rand() % 2)
                                value = value + (float)(rand() % 100 + 1) / 10;
                            else
                                value = value - (float)(rand() % 100 + 1) / 10;
                            userTask(myname, hostname, port, filename, t, value);
                            sleep(1);
                        }
                        value = temp;
                    }
                    else {
                        if (isdigit(check[0]) || atoi(check) < 0)
                            printf("<n> must be a valid integer greater than 0\n");
                        else
                            printf("%s: random <n> is wrong\n", check);
                    }
                }
                else
                    printf("Please enter <n>\n");
            }

            else if (!strcmp(command, "prand")) {
                if (*check) {
                    if (atoi(check) > 0) {
                        random = atoi(check);
                        pthread_t* thread = (pthread_t*)malloc(sizeof(pthread_t) * random);
                        for (int i = 0; i < random; i++) {
                            float temp = value;
                            pthread_create(&thread[i], NULL, producer, NULL);
                            value = temp;
                        }
                    }
                    else {
                        if (isdigit(check[0]) || atoi(check) < 0)
                            printf("<n> must be a valid integer greater than 0\n");
                        else
                            printf("%s: random <n> is wrong\n", check);
                    }
                }
                else
                    printf("Please enter <n>\n");
            }

            else if (!strcmp(command, "quit")) {
                if (!*check)
                    break;
                else
                    printf("quit: no quit topics match '%s'\n", check);
            }

            else
                printf("%s: command not found\n", command);
        }
        else
            printf("The number of commands must be less than 2\n");
    }
}

void* producer(void* arg) {
    long int start_t = time(NULL);
    struct timespec begin, end;
    char *msg;

    clock_gettime(CLOCK_MONOTONIC, &begin);

    long int t = start_t * 1000000 + (begin.tv_sec + begin.tv_nsec) / 1000;
    printf("Thread 시작 시간: %ld\n", t);

    if (rand() % 2)
        value = value + (float)(rand() % 100 + 1) / 10;
    else
        value = value - (float)(rand() % 100 + 1) / 10;

    userTask(myname, hostname, port, filename, (double)t / 1000000, value);

    clock_gettime(CLOCK_MONOTONIC, &end);
    t = start_t * 1000000 + (double)(end.tv_sec + end.tv_nsec) / 1000;
    printf("Thread 응답 시간: %ld\n", t);
}

int main(void)
{
    getargs_cp();

    command_shell();

    return(0);
}