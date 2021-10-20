#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>

//
// This program is intended to help you test your web server.
// 

int main(int argc, char *argv[])
{
  char buf[MAXBUF];
  char len[MAXBUF];
  int lens;
  
  sprintf(len, "%s", Getenv("CONTENT_LENGTH"));
  if(!len)
    return (-1);
  lens = atoi(len);
  
  Read(STDIN_FILENO, buf, lens);
  
  printf("HTTP/1.0 200 OK\r\n");
  printf("Server: My Web Server\r\n");
  printf("Content-Length: %d\r\n", atoi(buf));
  printf("Content-Type: text/plain\r\n\r\n");
  printf("Request-Body: %s\n", buf);
  fflush(stdout);
  
  return(0);
}
