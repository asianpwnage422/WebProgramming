#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<pthread.h>
#include<errno.h>

#define MESSAGE_BUFF 500

#define USERNAME_BUFF 10

typedef struct {

char* prompt;

int socket;

}thread_data;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//與server連線

void* connect_to_server(int socket_fd, struct sockaddr_in *address)

{

int response = connect(socket_fd, (struct sockaddr *) address, sizeof *address);

if (response < 0) {

fprintf(stderr, "connect() failed: %s\n", strerror(errno));

exit(1);

}else {

printf("Connected\n");

}

}

//打字、傳送訊息

void* send_message(char prompt[USERNAME_BUFF+4], int socket_fd, struct sockaddr_in *address, char username[])

{

    char message[MESSAGE_BUFF];

    char buff[MESSAGE_BUFF];

    char notice[]="/send";

    FILE *fp;

    memset(message, '\0', sizeof(message));

    memset(buff, '\0', sizeof(buff));

    send(socket_fd, username, strlen(username), 0);

    while(fgets(message, MESSAGE_BUFF, stdin)!=NULL) {

        printf("%s",prompt);

        if(strncmp(message, "/quit", 5)==0) {

        printf("Close connection...\n");

        exit(0);

        }

        send(socket_fd, message, strlen(message), 0);

        memset(message, '\0', sizeof(message));

    }

}

void* receive(void* threadData)

{

    int socket_fd, response;

    char message[MESSAGE_BUFF];

    thread_data* pData = (thread_data*)threadData;

    socket_fd = pData->socket;

    char* prompt = pData->prompt;

    char buff[MESSAGE_BUFF];

    char buff2[2];

    FILE *fp;

    char catch[]="<SERVER> Download...\n";

    char confirm[]="<SERVER> Confirm?";

    char ok[]="ok";

    //接收訊息

    while(1) {

        memset(message, '\0', MESSAGE_BUFF);

        memset(buff, '\0', sizeof(buff));

        fflush(stdout);

        response = recv(socket_fd, message, MESSAGE_BUFF, 0);

        if (response == -1) {

            fprintf(stderr, "recv() failed: %s\n", strerror(errno));

            break;

        }

        else if (response == 0) {

            printf("\nPeer disconnected\n");

            break;

        }

        //傳檔案

        if(strcmp(message, confirm)==0) {

            fp=fopen("./send/owo.txt", "r");

            fgets(buff, MESSAGE_BUFF, fp);

            fclose(fp);

            send(socket_fd, buff, strlen(buff), 0);

        }

        //download situation
        else if(strncmp(message, catch, 20)==0) {

            send(socket_fd, ok, 2, 0);

            fp=fopen("./receive/owo.txt", "w");

            pthread_mutex_lock(&mutex);

            recv(socket_fd, buff, MESSAGE_BUFF, 0);

            pthread_mutex_unlock(&mutex);

            fprintf(fp, "%s", buff);

            fclose(fp);

        }

        else {

            printf("%s", message);

            printf("%s", prompt);

            fflush(stdout);

        }

    }

}

int main(int argc, char**argv)

{

    long port = 8080;

    struct sockaddr_in address, cl_addr;

    char * server_address;

    int socket_fd, response;

    char prompt[USERNAME_BUFF+4];

    char username[USERNAME_BUFF];

    pthread_t thread;

    //進入大廳

    printf("Enter your name: ");

    fgets(username, USERNAME_BUFF, stdin);

    username[strlen(username) - 1] = 0;

    strcpy(prompt, username);

    strcat(prompt, "> ");

    //連線設定

    server_address = "127.0.0.1";

    address.sin_family = AF_INET;

    address.sin_addr.s_addr = inet_addr(server_address);

    address.sin_port = htons(port);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    connect_to_server(socket_fd, &address);

    //建thread data

    thread_data data;

    data.prompt = prompt;

    data.socket = socket_fd;

    //建thread接收訊息

    pthread_create(&thread, NULL, (void *)receive, (void *)&data);

    //傳送訊息

    send_message(prompt, socket_fd, &address,username);

    //關閉

    close(socket_fd);

    pthread_exit(NULL);

    return 0;

}

