/*
 * clientGet.c: A very, very primitive HTTP client for console.
 *
 * To run, prepare config-cg.txt and try:
 *      ./clientGet
 *
 * Sends one HTTP request to the specified HTTP server.
 * Prints out the HTTP response.
 *
 * For testing your server, you will want to modify this client.
 *
 * When we test your server, we will be using modifications to this client.
 *
 */


#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <semaphore.h>
#include "stems.h"

 /*
  * Send an HTTP request for the specified file
  */
     void clientSend(int fd, char* filename)
 {
     char buf[MAXLINE];
     char hostname[MAXLINE];

     Gethostname(hostname, MAXLINE);

     /* Form and send the HTTP request */
     sprintf(buf, "GET %s HTTP/1.1\n", filename);
     sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
     Rio_writen(fd, buf, strlen(buf));
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
         printf("Header: %s", buf);
         n = Rio_readlineb(&rio, buf, MAXBUF);

         /* If you want to look for certain HTTP tags... */
         if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
             printf("Length = %d\n", length);
         }
     }

     /* Read and display the HTTP Body */
     n = Rio_readlineb(&rio, buf, MAXBUF);
     while (n > 0) {
         printf("%s", buf);
         n = Rio_readlineb(&rio, buf, MAXBUF);
     }
 }

 /* currently, there is no loop. I will add loop later */
 void userTask(char hostname[], int port, char webaddr[])
 {
     int clientfd;

     clientfd = Open_clientfd(hostname, port);
     clientSend(clientfd, webaddr);
     clientPrint(clientfd);
     Close(clientfd);
 }

 void getargs_cg(char hostname[], int* port)
 {
     FILE* fp;

     fp = fopen("config-cg.txt", "r");
     if (fp == NULL)
         unix_error("config-cg.txt file does not open.");

     fscanf(fp, "%s", hostname);
     fscanf(fp, "%d", port);
     fclose(fp);
 }


 void command_shell(char* hostname[], int port)
 {
     while (1) {
         char str[MAXLINE], command[MAXLINE], sname[MAXLINE] = { NULL }, n[MAXLINE] = { NULL };
         char webaddr[MAXLINE] = "/dataGet.cgi?";
         char query[MAXLINE];
         char* tok;

         printf("#");
         scanf("%[^\n]s%", str);
         getchar();

         tok = strtok(str, " ");
         strcpy(command, tok);

         int tok_num = 0;
         while (tok != NULL) {
             tok = strtok(NULL, " ");
             if (tok_num == 0 && tok != NULL)
                 strcpy(sname, tok);
             else if(tok_num == 1 && tok != NULL)
                 strcpy(n, tok);
             tok_num++;
         }

         if (tok_num <= 3) {

             if (!strcmp(command, "LIST")) {
                 if (!*sname) { // 명령어 확인 LIST 뒤에 명령어가 더 없으면 ok
                     sprintf(query, "command=%s", command);
                     strcat(webaddr, query);
                     userTask(hostname, port, webaddr);
                 }
                 else
                     printf("%s: value <n> is wrong\n", sname);
             }

             else if (!strcmp(command, "INFO")) {
                 if (*sname) { // 명령어 확인 INFO 뒤에 명령어가 있을 경우 ok
                     sprintf(query, "command=%s&value=%s", command, sname);
                     strcat(webaddr, query);
                     userTask(hostname, port, webaddr);
                     printf("%s", Getenv("GET_REQUEST"));
                 }
                 else
                     printf("Please enter <sname>\n");
             }

             else if (!strcmp(command, "GET")) {
                 if (*sname) { // 명령어 확인 INFO 뒤에 명령어가 있을 경우 ok
                     if (*n) { // 명령어 확인 n이 존재하면 최근 data  n개 확인
                         if (atoi(n)) { // n이 숫자인지 판단
                             sprintf(query, "NAME=%s&N=%s", sname, n);
                             strcat(webaddr, query);
                             userTask(hostname, port, webaddr);
                             printf("%s", Getenv("GET_REQUEST"));
                         }
                         else if (isdigit(n[0])) {
                             printf("The number of <n> must exceed zero\n");
                         }
                         else
                             printf("%s: value <n> is wrong\n", n);
                     }
                     else { // 최근 data 1개 확인
                         sprintf(query, "NAME=%s&N=1", sname);
                         strcat(webaddr, query);
                         userTask(hostname, port, webaddr);
                     }
                 }
                 else
                     printf("Please enter <sname>\n");
             }

             else if (!strcmp(command, "QUIT") || !strcmp(command, "EXIT")) {
                 if (!*sname) // 명령어 확인 QUIT나 EXIT 뒤에 명령어가 없을 경우 ok
                     break;
                 else
                     printf("%s: no topics match '%s'\n", sname, sname);
             }
             else
                 printf("%s: command not found\n", command);
         }
         else
             printf("The number of commands must be less than 3\n");
     }
 }

 int main(void)
 {
     char hostname[MAXLINE];
     int port;

     getargs_cg(hostname, &port);

     command_shell(hostname, port);

     return(0);
 }
