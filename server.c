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
#include "proto.h"
#include "server.h"

// Global variables
int server_sockfd = 0, client_sockfd = 0;
ClientList *root, *now;

int Commands(char str[])
{
    char *nick = "/nick";
    char *create = "/create";
    char *remove = "/remove";
    char *list = "/list";
    char *join = "/join";
    char *part = "/part";
    char *names = "/names";
    char *kick = "/kick";
    char *msg = "/msg";
    char *quit = "/quit";

    char AllMsg[201];

    strcpy(AllMsg,str);

    char* comm = strtok(str," ");

    if(comm!=NULL){
        if(strcmp(comm,nick)==0){
            printf("The command is nick!!");
            return 0;
        }
        if(strcmp(comm,create)==0){
            printf("The command is create!!");
            return 1;
        }
        if(strcmp(comm,remove)==0){
            printf("The command is remove!!");
            return 2;
        }
        if(strcmp(comm,list)==0){
            printf("The command is list!!");
            return 3;
        }
        if(strcmp(comm,join)==0){
            printf("The command is join!!");
            return 4;
        }
        if(strcmp(comm,part)==0){
            printf("The command is part!!");
            return 5;
        }
        if(strcmp(comm,names)==0){
            printf("The command is names!!");
            return 6;
        }
        if(strcmp(comm,kick)==0){
            printf("The command is kick!!");
            return 7;
        }
        if(strcmp(comm,msg)==0){
            printf("The command is msg!!");
            return 8;
        }
        if(strcmp(comm,quit)==0){
            printf("The command is quit!!");
            return 9;
        }
        else
        {
            return -1;
        }
    }
    
    
}

void catch_ctrl_c_and_exit(int sig) {
    ClientList *tmp;
    while (root != NULL) {
        printf("\nClose socketfd: %d\n", root->data);
        close(root->data); // close all socket include server_sockfd
        tmp = root;
        root = root->link;
        free(tmp);
    }
    printf("Bye\n");
    exit(EXIT_SUCCESS);
}

void send_to_all_clients(ClientList *np, char tmp_buffer[]) {
    ClientList *tmp = root->link;
    char* comm;
    char buffer[201];
    strcpy(buffer,tmp_buffer);
    while (tmp != NULL) {
        if (np->data != tmp->data) { // all clients except itself.
            printf("Send to sockfd %d: \"%s\" \n", tmp->data, tmp_buffer);
            switch(Commands(tmp_buffer)){
                case 0: // nick
                comm = strtok(buffer," ");
                comm = strtok(NULL," ");
                printf("New name is: %s\n",comm);
                break;

                case 1: // create

                break;

                case 2: //remove

                break;

                case 3: //list

                break;

                case 4: //join

                break;

                case 5: //part

                break;

                case 6: //names

                break;

                case 7: //kick

                break;

                case 8: //msg
                send(tmp->data, tmp_buffer, LENGTH_SEND, 0);
                break;

                case 9: //quit

                break;

                default:
                ;


            }
            
            
            
            if(Commands(tmp_buffer)==-1){
                
            }
            
        }
        tmp = tmp->link;
    }
}

void client_handler(void *p_client) {
    int leave_flag = 0;
    char nickname[LENGTH_NAME] = {};
    char recv_buffer[LENGTH_MSG] = {};
    char send_buffer[LENGTH_SEND] = {};
    ClientList *np = (ClientList *)p_client;

    // Naming
    if (recv(np->data, nickname, LENGTH_NAME, 0) <= 0 || strlen(nickname) < 2 || strlen(nickname) >= LENGTH_NAME-1) {
        printf("%s didn't input name.\n", np->ip);
        leave_flag = 1;
    } else {
        strncpy(np->name, nickname, LENGTH_NAME);
        printf("%s(%s)(%d) joined the chatroom.\n", np->name, np->ip, np->data);
        sprintf(send_buffer, "%s(%s) joined the chatroom.", np->name, np->ip);
        send_to_all_clients(np, send_buffer);
    }

    // Conversation
    while (1) {
        if (leave_flag) {
            break;
        }
        int receive = recv(np->data, recv_buffer, LENGTH_MSG, 0);
        if (receive > 0) {
            if (strlen(recv_buffer) == 0) {
                continue;
            }
            sprintf(send_buffer, "%s",recv_buffer);
        } else if (receive == 0 || strcmp(recv_buffer, "exit") == 0) {
            printf("%s(%s)(%d) leaved the chatroom.\n", np->name, np->ip, np->data);
            sprintf(send_buffer, "%s(%s) leaved the chatroom.", np->name, np->ip);
            leave_flag = 1;
        } else {
            printf("Fatal Error: -1\n");
            leave_flag = 1;
        }
        send_to_all_clients(np, send_buffer);
    }

    // Remove Node
    close(np->data);
    if (np == now) { // remove an edge node
        now = np->prev;
        now->link = NULL;
    } else { // remove a middle node
        np->prev->link = np->link;
        np->link->prev = np->prev;
    }
    free(np);
}

int main()
{
    signal(SIGINT, catch_ctrl_c_and_exit);

    // Create socket
    server_sockfd = socket(AF_INET , SOCK_STREAM , 0);
    if (server_sockfd == -1) {
        printf("Fail to create a socket.");
        exit(EXIT_FAILURE);
    }

    // Socket information
    struct sockaddr_in server_info, client_info;
    int s_addrlen = sizeof(server_info);
    int c_addrlen = sizeof(client_info);
    memset(&server_info, 0, s_addrlen);
    memset(&client_info, 0, c_addrlen);
    server_info.sin_family = PF_INET;
    server_info.sin_addr.s_addr = INADDR_ANY;
    server_info.sin_port = htons(8888);

    // Bind and Listen
    bind(server_sockfd, (struct sockaddr *)&server_info, s_addrlen);
    listen(server_sockfd, 5);

    // Print Server IP
    getsockname(server_sockfd, (struct sockaddr*) &server_info, (socklen_t*) &s_addrlen);
    printf("Started Server on: %s:%d\n", inet_ntoa(server_info.sin_addr), ntohs(server_info.sin_port));

    // Initial linked list for clients
    root = newNode(server_sockfd, inet_ntoa(server_info.sin_addr));
    now = root;

    while (1) {
        client_sockfd = accept(server_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);

        // Print Client IP
        getpeername(client_sockfd, (struct sockaddr*) &client_info, (socklen_t*) &c_addrlen);
        printf("Client %s:%d came in.\n", inet_ntoa(client_info.sin_addr), ntohs(client_info.sin_port));

        // Append linked list for clients
        ClientList *c = newNode(client_sockfd, inet_ntoa(client_info.sin_addr));
        c->prev = now;
        now->link = c;
        now = c;

        pthread_t id;
        if (pthread_create(&id, NULL, (void *)client_handler, (void *)c) != 0) {
            perror("Create pthread error!\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
