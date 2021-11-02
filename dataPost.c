#include "stems.h"
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include "/usr/include/mysql/mysql.h"  //mysql include

void insertdatabase(MYSQL *conn, char* name, char* value, char* time){
  MYSQL_RES *res; //mysql의 결과 한줄을 저장하는 변수
  MYSQL_ROW row;  //mysql데이터 하나를 저장하는 변수
  int s_number = 1;  //sensor 테이블을 구분하는 변수
  char sensor[MAXLINE] = "sensor";  //sensor 테이블 앞에 붙을 문자열
  char s_num[5];  //s_number를 넣을 배열
  char query[MAXLINE];  //mysql_query와 sprintf를 이용하기위한 배열
  int sec = 1;  //값을 최초로 넣는지 아닌지 구분하기 위한 변수
  
  if(mysql_query(conn, "SELECT * FROM sensorList")){  //sensorList 테이블 생성
    mysql_query(conn, "CREATE TABLE sensorList (name varchar(80) not null, id int(2) not null AUTO_INCREMENT, \
        cnt int(2) not null, ave float not null, max float not null, PRIMARY KEY (id))AUTO_INCREMENT = 1;");
    mysql_query(conn, "SELECT * FROM sensorList");
  }
  
  res = mysql_store_result(conn); //mysql에 나온 결과를 저장한다.
  
  while((row = mysql_fetch_row(res))){ //나온 결과를 한줄씩 불러와 단어 하나하나로 나눈다.   //값이 없으면 0 있으면 포인터값
    if(!strcasecmp(row[0], name)){  //첫 단어가 name과 같을 경우
      sec = 0;
      break;
    }
    s_number++;
  }
  
  sprintf(s_num, "%d", s_number);
  strcat(sensor, s_num);  //sensor 문자열과 s_number를 결합
  
  if(sec == 0){
    sprintf(query, "INSERT INTO %s VALUES('%s', %s, NULL)", sensor, time, value);  //sensorx 테이블에 값을 넣음
    mysql_query(conn, query);
    
    sprintf(query, "UPDATE sensorList set cnt = cnt + 1 WHERE name = '%s'", name);  //sensorList 테이블의 cnt 값을 1 올림
    mysql_query(conn, query);
    sprintf(query, "UPDATE sensorList SET ave = (SELECT AVG(value) FROM %s) WHERE name = '%s'", sensor, name); //sensorList 테이블의 ave에 평균값 저장
    mysql_query(conn, query);
    sprintf(query, "UPDATE sensorList SET max = IF(max < %s, %s, max) WHERE name = '%s'", value, value, name); //sensorList 테이블의 max에 최대값 저장
    mysql_query(conn, query);
  }
  else{ //sec == 1 최초로 값을 집어넣을 경우
    sprintf(query, "CREATE TABLE %s (time varchar(15) not null, value float not null, idx int(2) not null AUTO_INCREMENT, \
        PRIMARY KEY (idx))AUTO_INCREMENT = 1;", sensor); //sensorx 테이블 생성
    mysql_query(conn, query);
    
    sprintf(query, "INSERT INTO %s VALUES('%s', %s, null)", sensor, time, value); //sensorx 테이블에 값을 넣음
    mysql_query(conn, query);
    
    sprintf(query, "INSERT INTO sensorList VALUES('%s', id, 1, %s, %s)", name, value, value);  //sensorList에 값을 넣음
    mysql_query(conn, query);
  }
  printf("200 OK\n");  //모든 값들을 데이터베이스에 저장하고 출력
}

void initdb(void){
  char buf[MAXBUF];
  char len[MAXBUF];
  int lens;
  
  sprintf(len, "%s", Getenv("CONTENT_LENGTH"));
  if(!len)
    return (-1);
  lens = atoi(len);
  
  /************database************/
  Read(STDIN_FILENO, buf, lens);  //buf값을 불러온다.
  
  char name[MAXLINE];  
  char value[MAXLINE];
  char time[MAXLINE];
  
  MYSQL *conn = mysql_init(NULL);  //mysql 초기화
  char password[MAXLINE] = "1234";  //mysql 비밀번호
  
  if (mysql_real_connect(conn,"127.0.0.1", "root", password, NULL , 0, NULL, 0) == NULL){  //mysql 연결
    mysql_error(conn);
  }
    
  if (mysql_query(conn, "USE PROJECT")){  //mysql콘솔창에 쓰는 명령어를 입력하는 함수, 성공하면 TRUE 실패하면 FALSE 반환
    mysql_query(conn, "CREATE DATABASE PROJECT");  //데이터베이스 생성
    mysql_query(conn, "USE PROJECT");
  }
  
  strtok(buf, "="); 
  sprintf(name, "%s", strtok(NULL, "&"));  //strtok를 이용하여 name 구분
  
  strtok(NULL, "=");
  sprintf(time, "%s", strtok(NULL, "&"));  //strtok를 이용하여 time 구분

  strtok(NULL, "=");
  sprintf(value, "%s", strtok(NULL, "&"));  //strtok를 이용하여 value 구분

  insertdatabase(conn, name, value, time); //구분한 값을 데이터베이스에 넣는 함수
  
  mysql_close(conn);
  
}

int main(int argc, char *argv[])
{
  initdb();
  return(0);
}
