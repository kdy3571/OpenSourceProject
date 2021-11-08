#include "stems.h"

void parseData(char *buf, char *name, char *time, char *value){
    /*받아온 값을 이름, 시간, 값으로 구분하는 함수*/
    strtok(buf, "="); 
    sprintf(name, "%s", strtok(NULL, "&")); 

    strtok(NULL, "=");
    sprintf(time, "%s", strtok(NULL, "&"));

    strtok(NULL, "=");
    sprintf(value, "%s", strtok(NULL, "&"));
}

int main(void){
    char buf[MAXLINE];
    char name[MAXLINE], time[MAXLINE], value[MAXLINE];
    
    char len[MAXBUF];
    int lens;
    sprintf(len, "%s", Getenv("CONTENT_LENGTH"));
    if(!len)
      return (-1);
    lens = atoi(len);
    Read(STDIN_FILENO, buf, lens); //buf 값을 불러온다
    parseData(buf, name, time, value);  //값을 분리

    fprintf(stderr, "경고 : %s sensor로부터 %s 시각에 %s 라는 값이 발생했습니다.\n", name, time, value);
    printf("200 OK\n");
    fflush(stdout);
    return 0;
}
