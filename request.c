//
// request.c: Does the bulk of the work for the web server.
// 

#include "stems.h"
#include "request.h"

/*  time related functions */
struct timeval startTime;

void initWatch(void)
{
  gettimeofday(&startTime, NULL);
}

double getWatch(void)
{
  struct timeval curTime;
  double tmp;

  gettimeofday(&curTime, NULL);
  tmp = (curTime.tv_sec - startTime.tv_sec) * 1000.0;
  return (tmp - (curTime.tv_usec - startTime.tv_usec) / 1000.0);
}

/* web request process routines */

void requestError(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) 
{
  char buf[MAXLINE], body[MAXBUF];

  printf("Request ERROR\n");

  // Create the body of the error message
  sprintf(body, "<html><title>CS537 Error</title>");
  sprintf(body, "%s<body bgcolor=""fffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr>CS537 Web Server\r\n", body);

  // Write out the header information for this response
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  printf("%s", buf);

  sprintf(buf, "Content-Type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  printf("%s", buf);

  sprintf(buf, "Content-Length: %lu\r\n\r\n", (long unsigned int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  printf("%s", buf);

  // Write out the content
  Rio_writen(fd, body, strlen(body));
  printf("%s", body);

}

//
// Reads and discards everything up to an empty text line
// If it encounters 'Content-Length:', it read the value for later use
//
void requestReadhdrs(rio_t *rp, int *length)
{
  char buf[MAXLINE];

  *length = -1;
  Rio_readlineb(rp, buf, MAXLINE);
  while (strcmp(buf, "\r\n")) {
    sscanf(buf, "Content-Length: %d", length);
    Rio_readlineb(rp, buf, MAXLINE);
  }
  return;
}

//
// Return STATIC if static, DYNAMIC if dynamic content
// Input: uri
// output: filename
//         cgiargs (in the case of dynamic)
//
int parseURI(char *uri, char *filename, char *cgiargs) 
{
  if (!strstr(uri, "cgi")) {
    // static
    strcpy(cgiargs, "");
    sprintf(filename, ".%s", uri);
    if (uri[strlen(uri)-1] == '/') {
      strcat(filename, "index.html");
    }
    return STATIC;
  } else {
    // dynamic
    /* seperate a uri to a filename and a cgiargs. For example,
       if uri is "/output.cgi?100&TEST" then filename is 
       "./output.cgi" and cgiargs is "100&TEST" (? is not copied)
    */
      char *pos;
      if(strstr(uri, "?"))
      {
      pos = strtok(uri, "?");
      sprintf(filename, ".%s", pos);
      //printf("filename: %s\n", filename);
      
      pos = strtok(NULL, "");
      sprintf(cgiargs, "%s", pos);
      //printf("cgiargs: %s\n", cgiargs);
      }
      else {
        sprintf(filename, ".%s", uri);
        strcpy(cgiargs, " ");
      }
    return DYNAMIC;
  }
}

//
// Fills in the filetype given the filename
//
void requestGetFiletype(char *filename, char *filetype)
{
  if (strstr(filename, ".html")) 
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif")) 
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".jpg")) 
    strcpy(filetype, "image/jpeg");
  else 
    strcpy(filetype, "test/plain");
}

// requestServeDynamic(&rio, connfd, filename, cgiargs, bodyLength, arrivalTime);
void requestServeDynamic(rio_t *rio, int fd, char *filename, char *cgiargs, int bodyLength, double arrivalTime)
{
  int pid;
  int pfd[2];
  char *method;
  char lens[MAXBUF];
  char buf[MAXBUF];
  char *argv[] = { NULL };

  method = getenv("REQUEST_METHOD");
  pid = Fork();
  if(pid == 0) {
    Setenv("QUERY_STRING", cgiargs, 1);
    Setenv("REQUEST_METHOD", method, 1);
    if (bodyLength > 0) {
      sprintf(lens, "%d", bodyLength+1);
      Setenv("CONTENT_LENGTH", lens, 1);
      Rio_readlineb(rio, buf, atoi(lens));
      Setenv("CONTENT_BODY", buf, 1);

      Pipe(pfd);
      Write(pfd[1], buf, bodyLength+1);
      Close(pfd[1]);
      Dup2(pfd[0], STDIN_FILENO);
      Close(pfd[0]);
    }
    Dup2(fd, STDOUT_FILENO);
    execve(filename, argv, environ);
  }
}


void requestServeStatic(int fd, char *filename, int filesize, double arrivalTime)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  // Rather than call read() to read the file into memory, 
  // which would require that we allocate a buffer, we memory-map the file
  requestGetFiletype(filename, filetype);
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);

  // put together response
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: My Web Server\r\n", buf);

  // Your statistics go here -- fill in the 0's with something useful!
  sprintf(buf, "%sStat-req-arrival: %lf\r\n", buf, arrivalTime);
  // Add additional statistic information here like above
  // ...
  //
  
  sprintf(buf, "%sContent-Length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-Type: %s\r\n\r\n", buf, filetype);

  Rio_writen(fd, buf, strlen(buf));

  // Writes out to the client socket the memory-mapped file 
  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

// handle a request
void requestHandle(int connfd, double arrivalTime)
{

  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  rio_t rio;
  int reqType;
  char filename[MAXLINE], cgiargs[MAXLINE];
  struct stat sbuf;
  int bodyLength;

  Rio_readinitb(&rio, connfd);
  Rio_readlineb(&rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, uri, version);
  printf("%s %s %s\n", method, uri, version);
  Setenv("REQUEST_METHOD", method, 1);
  
  requestReadhdrs(&rio, &bodyLength);
  reqType = parseURI(uri, filename, cgiargs);
  if ((strcasecmp(method, "GET")!=0)&&(strcasecmp(method,"POST")!=0)){
    requestError(connfd, method, "501", "Not Implemented",
		 "My Server does not implement this method");
    return;
  }

  if (stat(filename, &sbuf) < 0) {
    Rio_readrestb(&rio, buf);
    requestError(connfd, filename, "404", "Not found", "My Server could not find this file");
    return;
  }

  if (reqType == STATIC) {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      requestError(connfd, filename, "403", "Forbidden", "My Server could not read this file");
      return;
    }
    requestServeStatic(connfd, filename, sbuf.st_size, arrivalTime);
  } else {
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      requestError(connfd, filename, "403", "Forbidden", "My Server could not run this CGI program");
      return;
    }
    requestServeDynamic(&rio, connfd, filename, cgiargs, bodyLength, arrivalTime);
  }
}
