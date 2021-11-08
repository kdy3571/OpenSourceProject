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
    
    strcpy(buf, getenv("CONTENT_BODY")); //환경변수에서 값 복사하기
    parseData(buf,name,time,value); //복사한 값 구분하기

    fprintf(stderr, "경고 : %s sensor로부터 %s 시각에 %s 라는 값이 발생했습니다.\n", name, time, value);
    printf("200 OK\n");
    fflush(stdout);
    return 0;
}
