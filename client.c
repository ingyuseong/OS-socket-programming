#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 100
#define LENGTH 2048
#define NAME_LEN 32

// Global variables
volatile sig_atomic_t flag = 0; // Terminate_flag
int sockfd = 0;
char name[NAME_LEN]; // Client name

void str_overwrite_stdout() {
  printf("%s", "> ");
  fflush(stdout);
}

void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

// 메세지를 입력받아 서버로 send하는 함수
void send_msg_handler() {
	char message[LENGTH] = {};  // 입력받은 메세지를 위한 버퍼
	char buffer[LENGTH + NAME_LEN] = {}; // 서버로 전송할 메세지를 위한 버퍼

	fflush(stdin);
	fgets(message, LENGTH, stdin);

	while(1) {
		// 사용자 입력
		str_overwrite_stdout();
		fgets(message, LENGTH, stdin);
		str_trim_lf(message, LENGTH);

		if (strcmp(message, "Q") == 0) {
			// Q (Exit Code) 입력 시, 쓰레드 종료
			break;
		} else {
			// 서버로 전송할 메세지
			// 'Client name: Message'의 format으로 메세지 전송
			sprintf(buffer, "%s : %s\n", name, message);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		bzero(message, LENGTH); // Clear the buffer
		bzero(buffer, LENGTH + 32); // Clear the buffer
	}
	
	catch_ctrl_c_and_exit(2); // 프로세스 종료
}

// 메세지를 receive하여 출력하는 함수
void recv_msg_handler() {
	char message[LENGTH] = {}; // 메세지 버퍼

	// Receive name from the server (Client N)
	recv(sockfd, message, LENGTH, 0);
	strcpy(name, message);
	memset(message, 0, sizeof(message)); // Clear the buffer
	
	while (1) {
		int receive = recv(sockfd, message, LENGTH, 0);

		if (receive > 0) {
			printf("%s", message);
			str_overwrite_stdout();
		} else if (receive == 0) {
			break;
		}

		memset(message, 0, sizeof(message)); // Clear the buffer
  }
}

int main(int argc, char **argv){

	// 기본적인 변수 선언 및 할당
	char *ip;
	int port;

	printf("Input Server IP Address : ");
	scanf("%s", ip);
	printf("Input Server Port Number : ");
	scanf("%d", &port);
	printf("IP : %s, Port : %d\n", ip, port);

	signal(SIGINT, catch_ctrl_c_and_exit);

	// Localhost가 아닌 경우 예외 처리
	if (strcmp(ip, "127.0.0.1") != 0) {
		printf("Wrong IP Address.\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr;

	/* 소켓 설정 및 연결하고자 하ㄴ 소켓 서버 설정 */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);


	// Connect to Server
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1) {
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	// Send name, need docs
	send(sockfd, ip, NAME_LEN, 0);

	printf("open!! client\n");
	printf("Success to connect to server!\nServer : welcome to chatting server!!\n Chatting On...\n");
	printf("Input \'Q\' to exit\n");

	// We need two threads. One for sending the message and one for receiving the message
	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0) {
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0) {
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1) {
		if (flag) { // Global variable
			printf("\nBye\n");
			break;
		}
	}

	close(sockfd);

	return EXIT_SUCCESS;
}
