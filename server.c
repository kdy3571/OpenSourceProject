#include "stems.h"

#include "request.h"

typedef struct{
	int fd;
	long time;
} data;

typedef struct{
	data *queue;
	int front, rear;
} Queue;

sem_t mutex, empty, full;
int N, P;
Queue q;
pthread_t *thread;


void getargs_ws(int *port, int *P, int *N)
{
  FILE *fp;
  if ((fp = fopen("config-ws.txt", "r")) == NULL)
    unix_error("config-ws.txt file does not open.");

  fscanf(fp, "%d", port);
  fscanf(fp, "%d", P);
  fscanf(fp, "%d", N);

  fclose(fp);
}

void consumer(int connfd, long arrivalTime)
{
  requestHandle(connfd, arrivalTime);
  Close(connfd);
}


void initQueue(Queue *q, int N){
	q->front = q->rear = 0;
	q->queue = (data *)malloc(sizeof(q->queue)*N);
}

void enqueue(Queue *q, data send){
	q->rear = (q->rear + 1) % N;
	q->queue[q->rear] = send;
}

data dequeue(Queue *q) {
	q->front = (q->front + 1) % N;
	return q->queue[q->front];
}

void SendData(int connfd, long time) { // producer : main thread
	sem_wait(&empty);
	sem_wait(&mutex);

	data temp;
	temp.fd = connfd; 
	temp.time = time;
	enqueue(&q, temp);

	sem_post(&mutex);
	sem_post(&full);
}

void ReceveData(int *connfd, long *time) { // consumer : worker thread
	sem_wait(&full);
	sem_wait(&mutex);

	data temp = dequeue(&q);
	*connfd = temp.fd;
	*time = temp.time;

	sem_post(&mutex);
	sem_post(&empty);
}

void Producer(void *ptr){
	int connfd;
	long time;
	ReceveData(&connfd, &time);
	while(1){
		consumer(connfd, time);
		ReceveData(&connfd, &time);
	}
}

int main(void)
{
  pid_t pid;
  int listenfd, connfd, port, clientlen;
  struct sockaddr_in clientaddr;

  initWatch();

  getargs_ws(&port, &P, &N);
  
  initQueue(&q, N);
  thread = (pthread_t*)malloc(sizeof(pthread_t)*P);
  
  sem_init(&mutex, 0, 1);
  sem_init(&full, 0, 0);
  sem_init(&empty, 0, N);
  listenfd = Open_listenfd(port);

  pid = fork(); //프로세스 생성

  if(pid == 0){ //자식
    Execve("./alarmClient", NULL, NULL);
  }
  else{
    while (1) { //부모
      for(int i = 0; i < P; i++){
        if(pthread_create(&thread[i], NULL, Producer, NULL)){
          printf("pthread_create() : error.\n");
          exit(1);
        }
      }
      while(1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        SendData(connfd, getWatch);
      }
      for(int i = 0; i < P; i++)
        pthread_detach(thread[i]);
    }
  }
  return(0);
}
