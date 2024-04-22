#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 1024

typedef struct room{
    int number;
    const char* info;
    int installed_sensors;
}room;

int rooms_control[8] = {0};
room rooms[8];

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct client_data {
    int csock;
    struct sockaddr_storage storage;
};

char* insert_sensors_data(char* info, char *opmode){
    char *message = "";
    char* room_number = strtok(NULL, " ");
    int number = atoi(room_number);
    if(rooms_control[number] == 0){
        char error_message[BUFSZ] = "ERROR 03";
        message = error_message;
    }
    else if (rooms[number].installed_sensors == 1 && strcmp(opmode, "INI_REQ") == 0){
        char error_message[BUFSZ] = "ERROR 05";
        message = error_message;
    }
    else if (rooms[number].installed_sensors == 0 && strcmp(opmode, "ALT_REQ") == 0){
        char error_message[BUFSZ] = "ERROR 06";
        message = error_message;
    }
    else{
        rooms[number].installed_sensors = 1;
        char* info = strtok(NULL, "\0");
        printf("info: %s\n", info);
        rooms[number].info = strdup(info);
        char register_message[BUFSZ] = "";
        memcpy(register_message, (strcmp(opmode, "INI_REQ") == 0) ? "OK 02" : "OK 04", sizeof(register_message));
        message = register_message;
    }
    return message;
}

char* parse_message(char *buf){

    char *command = strtok(buf, " ");
    char *message = "";

    if (strcmp(command, "CAD_REQ") == 0){

        char* room_number = strtok(NULL, " ");
        int number = atoi(room_number);

        if(rooms_control[number] != 0){
            char error_message[BUFSZ] = "ERROR 02";
            message = error_message;
        }
        else{
            rooms_control[number] = 1;
            room new_room;
            new_room.number = number;
            new_room.info = strdup("-1 -1 -1 -1 -1 -1");
            new_room.installed_sensors = 0;
            rooms[number] = new_room;
            char register_message[BUFSZ] = "OK 01";
            message = register_message;
        }
    }

    else if (strcmp(command, "INI_REQ") == 0){
        message = insert_sensors_data(buf, command);
    }

    else if (strcmp(command, "DES_REQ") == 0){
        char* room_number = strtok(NULL, " ");
        int number = atoi(room_number);
        if(rooms_control[number] == 0){
            char error_message[BUFSZ] = "ERROR 03";
            message = error_message;
        }
        else if(rooms[number].installed_sensors == 0){
            char error_message[BUFSZ] = "ERROR 06";
            message = error_message;
        }
        else{
            rooms[number].info = strdup("-1 -1 -1 -1 -1 -1");
            rooms[number].installed_sensors = 0;
            char register_message[BUFSZ] = "OK 03";
            message = register_message;
        }
    }

    else if (strcmp(command, "ALT_REQ") == 0){
        message = insert_sensors_data(buf, command);
    }

    else if (strcmp(command, "LOAD_REQ") == 0){
        char* room_number = strtok(NULL, " ");
        int number = atoi(room_number);
        if((number > 7 || number < 0) || rooms_control[number] == 0){
            char error_message[BUFSZ] = "ERROR 03";
            message = error_message;
        }
        else if (rooms[number].installed_sensors == 0){
            char error_message[BUFSZ] = "ERROR 06";
            message = error_message;
        }
        else{
            char register_message[BUFSZ] = "";
            printf("sala %d: %s", rooms[0].number, rooms[0].info);
            sprintf(register_message, "sala %d: %s", rooms[number].number, rooms[number].info);
            message = register_message;
        }
    }

    else if (strcmp(command, "INF_REQ") == 0){
        char* register_message = malloc(1024);
        register_message = "salas:";
        int empty = 1;
        for (int i = 0; i < 7; i++){
            if(rooms_control[i] == 1){
                empty = 0;
                break;            
            }
        }
        if(empty){
            char error_message[BUFSZ] = "ERROR 03";
            message = error_message;
        }
        else{
            for (int i = 0; i < 7; i++){
                if(rooms_control[i] == 1){
                    char *info = "";
                    sprintf(info, " %d (%s)", rooms[i].number, rooms[i].info);
                    strcat(register_message, info);
                }
            }
            message = register_message;
        }
    }
    return message;
}

void * client_thread(void *data) {
    struct client_data *cdata = (struct client_data *)data;
    struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);

    char caddrstr[BUFSZ];
    addrtostr(caddr, caddrstr, BUFSZ);
    printf("[log] connection from %s\n", caddrstr);

    while (1){
        char buf[BUFSZ];
        char* message;
        memset(buf, 0, BUFSZ);
        memset(buf, 0, BUFSZ);
        size_t count = recv(cdata->csock, buf, BUFSZ - 1, 0);
        printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        message = parse_message(buf);
        count = send(cdata->csock, message, strlen(message) + 1, 0);

        if (count < 1) {
            logexit("send");
        }
    }

    close(cdata->csock);
    pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

	struct client_data *cdata = malloc(sizeof(*cdata));
	if (!cdata) {
		logexit("malloc");
	}
	cdata->csock = csock;
	memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}
