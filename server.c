#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define NAME_LEN 32

// Static variable for the total number of clients
static _Atomic unsigned int cli_count = 0;
static _Atomic unsigned int cli_number = 0;
// Static variable for uid
static int uid = 10;

/* Client structure */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[NAME_LEN];
} client_t;

// Client pointer queue (Add clients into the queue)
client_t *clients[MAX_CLIENTS];

// To transfer sended messages between the clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout() {
    printf("\r%s", "> ");
	// Flush standard output
    fflush(stdout);
}

// Trim \n
void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void print_client_addr(struct sockaddr_in addr){
	// Print the IP address of the client
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr  & 0xff000000) >> 24);
}

/* Add clients to queue */
void queue_add(client_t *cl){
	// Mutual Exclusion을 통한 쓰레드의 Synchronization 구현
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]) {
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Remove clients to queue */
void queue_remove(int uid) {
	// Mutual Exclusion을 통한 쓰레드의 Synchronization 구현
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Send message to all clients except sender */
void send_message(char *s, int uid) {
	// Mutual Exclusion을 통한 쓰레드의 Synchronization 구현
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			// If it is existed client
			if(clients[i]->uid != uid){
				// If it is not the sender
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Send name to the client that has newly joined */
void send_name(char *s, int uid) {
	// Mutual Exclusion을 통한 쓰레드의 Synchronization 구현
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			// If it is existed client
			if(clients[i]->uid == uid){
				// If it is the sender
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* 클라이언트와의 모든 커뮤니케이션을 처리하는 함수 */
void *handle_client(void *arg) {
	char buff_out[BUFFER_SZ]; // 메세지
	char status[NAME_LEN];
	int leave_flag = 0; // 해당 클라이언트 leave 여부

	cli_count++;
	cli_number++;
	client_t *cli = (client_t *)arg;

	// 클라이언트 이름 생성 / 출력 / 전송
	// 클라이언트에서 보내는 메세지 리스닝
	if(recv(cli->sockfd, status, NAME_LEN, 0) <= 0 || strcmp(status, "127.0.0.1") != 0) {
		// 고정적으로 localhost에서 실행되기 때문에 접속 오류가 없는지 확인
		printf("Wrong connection.\n");
		leave_flag = 1;
	} else {
		// 첫 번째 메세지 성공적으로 receive
		snprintf(cli->name, NAME_LEN, "Client %d", cli_number); // 클라이언트 이름 생성
		sprintf(buff_out, "connected to %s\n", cli->name); // 서버 알림 메세지 생성
		printf("%s", buff_out); // 서버 알림 메세지 출력
		send_name(cli->name, cli->uid); // 클라이언트로 이름 전송
		bzero(buff_out, BUFFER_SZ); // Clear the buffer
		sprintf(buff_out, "%s has been connected!\n", cli->name);  // 클라이언트 알림 메세지 생성
		send_message(buff_out, cli->uid); // 클라이언트 알림 메세지 출력
	}

	bzero(buff_out, BUFFER_SZ); // Clear the buffer

	while(1){
		if (leave_flag) {
			break;
		}

		// Receive the message from the client
		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive > 0) {
			if(strlen(buff_out) > 0){
				// If it receives messages
				send_message(buff_out, cli->uid);

				str_trim_lf(buff_out, strlen(buff_out));
				printf("message from %s\n", buff_out);
			}
		} else if (receive == 0 || strcmp(buff_out, "Q") == 0) {
			// If the client wanna exit the server(chatroom)
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message(buff_out, cli->uid);
			leave_flag = 1; // Escape the loop
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1; // Escape the loop
		}

		bzero(buff_out, BUFFER_SZ); // Clear the buffer
	}

	/* Delete client from queue and yield thread */
	close(cli->sockfd); // Close the socket file descriptor
	queue_remove(cli->uid); // Remove the client from the queue
	free(cli); // Free memory allocation
	cli_count--;
	
	// Delete the thread
	pthread_detach(pthread_self());

	return NULL;
}

int main(){

	// 기본적인 변수 선언 및 할당
	char *ip = "127.0.0.1";
	int port;
	
	int option = 1;

	int listenfd = 0, connfd = 0;

	// 소켓 주소(Server, Client), Thread ID
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	// 포트 번호 입력
	printf("Server Port : ");
	scanf("%d", &port);

	/* 소켓 설정 */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);

	/* Ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0) {
		perror("ERROR: setsockopt failed");
		return EXIT_FAILURE;
	}

	/* 소켓: Bind */
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR: Socket binding failed");
		return EXIT_FAILURE;
	}

	/* 소켓: Listen */
	if (listen(listenfd, 10) < 0) {
		perror("ERROR: Socket listening failed");
		return EXIT_FAILURE;
	}

	// 서버 소켓의 성공적인 동작 수행: Greeting Message
	printf("open!! server\nChatting on\n");

	// 무한 반복문: Client 접속 리스닝
	while(1){
		socklen_t clilen = sizeof(cli_addr);
		// If Client connects -> Accept the connection
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* 사전에 설정해둔 최대 클라이언트 수를 초과하는지 확인 */
		if((cli_count + 1) == MAX_CLIENTS){
			// 초과 시 reject
			printf("Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		/* 순서대로 동작
			1. 클라이언트 설정 정의
			2. 사전에 정의해둔 큐에 클라이언트 추가
			3. 생성된 클라이언트를 핸들링하기 위한 새로운 쓰레드 생성
			4. void handle_client() 구현
		*/

		/* 클라이언트 설정 정의 */
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		/* 사전에 정의해둔 큐에 클라이언트 추가 및 쓰레드 생성 -> handle_client */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	return EXIT_SUCCESS;
}
