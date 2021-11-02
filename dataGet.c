#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
//
// This program is intended to help you test your web server.
// You can use it to test that you are correctly having multiple 
// threads handling http requests.
//
// htmlReturn() is used if client program is a general web client
// program like Google Chrome. textReturn() is used for a client
// program in a embedded system.
//
// Standalone test:
// # export QUERY_STRING="name=temperature&time=3003.2&value=33.0"
// # ./dataGet.cgi

void htmlReturn(void)

{
  char content[MAXLINE];
  char *buf;
  char *ptr;
  
  /* Make the response body */
  sprintf(content, "%s<html>\r\n<head>\r\n", content);
  sprintf(content, "%s<title>CGI test result</title>\r\n", content);
  sprintf(content, "%s</head>\r\n", content);
  sprintf(content, "%s<body>\r\n", content);
  sprintf(content, "%s<h2>Welcome to the CGI program</h2>\r\n", content);
  buf = getenv("QUERY_STRING");
  sprintf(content,"%s<p>Env : %s</p>\r\n", content, buf);
  ptr = strsep(&buf, "&");
  while (ptr != NULL){
    sprintf(content, "%s%s\r\n", content, ptr);
    ptr = strsep(&buf, "&");
  }
  sprintf(content, "%s</body>\r\n</html>\r\n", content);

  /* Generate the HTTP response */
  printf("Content-Length: %d\r\n", strlen(content));
  printf("Content-Type: text/html\r\n\r\n");
  printf("%s", content);
  fflush(stdout);
}

void textReturn(int *argc, char *argv[])
{
  char content[MAXLINE];
  char *buf;
  char *ptr;
  int index = 0;

  buf = getenv("QUERY_STRING");
  ptr = strsep(&buf, "&");
  while (ptr != NULL){
    sprintf(content, "%s\n", ptr);
    argv[index] = ptr;
    ptr = strsep(&buf, "&");
    index++;
  }
  *argc = index;
}

void getLIST(MYSQL *conn, int argc, char *argv[],char *content)
{
  MYSQL_RES *res;
  MYSQL_ROW row;

  if(mysql_query(conn,"SELECT * FROM sensorList"))
    mysql_error_detect(conn);
  
  if((sql_result = mysql_store_result(conn))==NULL)
    mysql_error_detect(conn);

    while((row = mysql_fetch_row(res)))
      sprintf(content,"%s%s ",content, row[0]);
    sprintf(content,"%s\n",content);
}

void getINFO(MYSQL *conn, int argc, char *argv[],char *content)
{
  MYSQL_RES *res;
  MYSQL_ROW row;

  float max = 0, ave = 0;
  char query[MAXLINE];
  char tok[MAXLINE];
  int cnt = 0;

  sscanf(argv[1],"value=%s",name);
  

  if(mysql_query(conn,"SELECT * FROM sensorList"))
    mysql_error_detect(conn);

  if((res = mysql_store_result(conn))==NULL)
    mysql_error_detect(conn);

  while((row = mysql_fetch_row(res))){
    if(!strcmp(row[0],name)){ 
      cnt = row[3];
      ave = row[4];
      max = row[5];
      break;
    }
  }
  
  sprintf(content,"%d %.1f %.1f \n", cnt, ave, max);
}

void getGET(MYSQL *conn, int argc, char *argv[], char *content)
{
  MYSQL_RES *res_slist, *res_sensor;
  MYSQL_ROW row_slist, row_sensor;
  char name[MAXLINE];
  int count; 
  char sensor[MAXLINE] = "sensor";
  char s_num[5];
  int s_number = 1;
  int sec = 1;
  char query[MAXLINE];
  int tableIndex = 0;
  time_t t_time;

  sscanf(argv[1], "NAME=%s", name);

  if(mysql_query(conn, "SELECT * FROM sensorList"))
    mysql_error_detect(conn);
    
  if((res_slist = mysql_store_result(conn))==NULL)
    mysql_error_detect(conn);

  while((row_slist = mysql_fetch_row(res_slist))){
    if(!strcmp(row_slist[0], name)){ 
      sec = 0;
      break;
    }
    s_number++;
  }
  if(sec == 1){
    sprintf(content, "** Cannot find sensor name;%s\n", name);
  } 
  else {
    sprintf(s_num, "%d", s_number);
    strcat(sensor, s_num);
    sprintf(query,"SELECT * FROM %s ORDER BY idx DESC",sensor);

    if(mysql_query(conn,query))
      mysql_error_detect(conn);

    if((res_sensor = mysql_store_result(conn))==NULL)
      mysql_error_detect(conn);

    sscanf(argv[2], "N=%d", &count);      
    while((row_sensor = mysql_fetch_row(res_sensor))){
        if(count < tableIndex) 
          break;
        t_time = row_sensor[0];
        sprintf(content,"%s %s\n", ctime(t_time), row_sensor[1]);
        tableIndex++;
    }
  }
}

void getDB(int argc, char *argv[],char *content)
{
  MYSQL *conn = mysql_init(NULL); 
  
  char password[MAXLINE] = "1234";
    
  if(conn==NULL){
    fprintf(stderr,"%s\n",mysql_error(conn));
    exit(1);
  }
  
  if(mysql_real_connect(conn,"localhost", "root", password, NULL, 0, NULL, 0) == NULL)
    mysql_error_detect(conn);
  
  if(mysql_query(conn, "USE PROJECT"))
    mysql_error_detect(conn);
  
  if(!strcasecmp("command=LIST",argv[0])) 
    getLIST(conn,argc,argv,content);
  else if(!strcasecmp("command=INFO",argv[0])) 
    getINFO(conn,argc,argv,content);
  else //GET
    getGET(conn,argc,argv,content);

  mysql_close(conn);
}

int main(void)
{
  int argc;
  char *argv[MAXLINE_ARGS];
  char content[MAXLINE];
  //htmlReturn();
  
  textReturn(&argc, argv);
  getDB(argc, argv, content);
  printf("%s", content);
  
  return(0);
}
