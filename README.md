# 2021 - 3-2 OpenSourceProject
> Rasberry-Pi를 이용한 IoT/임베디드 웹 시스템 구현

## 개요
- IoT를 이용한 센서 프로그램, 센서 정보를 저장하는 서버 프로그램, 저장된 정보를 열람하는 모니터 프로그램으로 구성
- 웹 서버는 여러 클라이언트들로 오는 데이터를 효과적으로 수신하기 위해 multi-thread 기능으로 구현
- HTTP protocol을 이용하여 통신

## 개발기간 및 환경
- 기간 : 2021.09 ~ 2022.12 (4개월)
- OS : Ubuntu(VirtualBox)
- 메인 언어 : C
- 태그 : ```MySQL```, ```C```, ```IoT```, ```Rasberry-Pi```

## 개발내용
- server.c : HTTP protocol을 처리함
- alarmClient.c alarmServer.c, alarm.c : 알람을 담당하는 파일. 사용자가 정한 임계 값의 온도나 습도가 초과하게 되면 서버에 Alarm을 보냄
- clientGet.c, clientPost.c : 클라이언트를 담당하는 파일. 사용자가 사용하는 명령어에 관련된 파일
- config-ws.txt : 서버의 포트번호, 큐의 크기, 스레드의 개수를 포함하는 txt
- dataGet.c, dataPost.c : Rasberry-Pi를 통해 받아온 데이터를 저장하고 송수신함.
- request.c : 서버에 요청을 위한 파일
- stems.c : 프로젝트 진행에 제공된 파일, hlelper 함수, RIO 패키지가 존재
