#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_ROOM 7
#define BUFSZ 1024

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

char* parse_message(char *buf){
    char* message = "";
	char *command;
    command = strtok(buf, " ");

	//REGISTER ROOM
    if (strcmp(command, "register") == 0){
		char* room_number = strtok(NULL, " ");
		int number = atoi(room_number);
		if(number > MAX_ROOM || number < 0){
			logerror(01);
		}
		else {
			char register_message[1024];
			sprintf(register_message, "CAD_REQ %d", number);
			message = register_message;
		}
    }

	//INIT ROOM FROM CLI/FILE
    else if (strcmp(command, "init") == 0){
        char* init_command = strtok(NULL, " ");
		char* info = malloc(1024);
		char* aux = malloc(1024);
		if(strcmp(init_command, "info") == 0){
			info = strtok(NULL, "\0");
			strcpy(aux, info);
		}
		else if (strcmp(init_command, "file") == 0){
			char* file = strtok(NULL, "\0");
			FILE *fp;
			fp = fopen(file, "r");
			if (fp == NULL){
				logexit("file not found");
			}
			fgets(info, 1024, fp);
			fclose(fp);

			strcpy(aux, info);
		}
		else{
			logexit("invalid command");
		}
		if (is_info_valid(info)){
			char init_message[1024];
			sprintf(init_message, "INI_REQ %s", aux);
			message = init_message;
		}
		//free(info);
		//free(aux);

    }

	//SHUT SENSORS DOWN
    else if (strcmp(command, "shutdown")== 0){
        char* room_number = strtok(NULL, " ");
		int number = atoi(room_number);
		if(number > MAX_ROOM || number < 0){
			logerror(01);
		}
		else {
			char shutdown_message[1024];
			sprintf(shutdown_message, "DES_REQ %d", number);
			message = shutdown_message;
		}
    }

	//UPDATE SENSORS FROM CLI/FILE
    else if (strcmp(command, "update") == 0){
        char* init_command = strtok(NULL, " ");
		char* info = malloc(1024);
		char* aux = malloc(1024);
		if(strcmp(init_command, "info") == 0){
			info = strtok(NULL, "\0");
			strcpy(aux, info);
		}
		else if (strcmp(init_command, "file") == 0){
			char* file = strtok(NULL, "\0");
			FILE *fp;
			fp = fopen(file, "r");
			if (fp == NULL){
				logexit("file not found");
			}
			fgets(info, 1024, fp);
			fclose(fp);

			strcpy(aux, info);
		}
		else{
			logexit("invalid command");
		}
		if (is_info_valid(info)){
			char update_message[1024];
			sprintf(update_message, "ALT_REQ %s", aux);
			message = update_message;
			printf("message: %s\n", message);
		}
		free(info);
		free(aux);
    }

	//LOAD INFO FROM A SINGLE ROOM OR ALL ROOMS
    else if (strcmp(command, "load")== 0){
		char * opmode = strtok(NULL, " ");
		printf("opmode: %s\n", opmode);
		//SINGLE ROOM
		if(strcmp(opmode, "info") == 0){
			char load_message[1024];
			char* room_number = strtok(NULL, " ");
			int number = atoi(room_number);
			sprintf(load_message, "LOAD_REQ %d", number);
			message = load_message;
		}
		//ALL ROOMS
		else if (strncmp(opmode, "rooms", 4) == 0){
			message = "INF_REQ";
		}
		else{
			logexit("invalid command");
		}

    }

    else{
        logexit("invalid command");
    }
	return message;
}

void parse_return(char *buf){
	char *command;
	command = strtok(buf, " ");
	if (strcmp(command, "OK") == 0){
		char* message = strtok(NULL, " ");
		logok(atoi(message));
	}
	else if (strcmp(command, "ERROR") == 0){
		char* message = strtok(NULL, " ");
		logerror(atoi(message));
	}
}

int main(int argc, char **argv) {
	if (argc < 3) {
		usage(argc, argv);
	}

	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(storage))) {
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("connected to %s\n", addrstr);

	while (1) {
		char buf[BUFSZ];	
		char* message = "";
		memset(buf, 0, BUFSZ);

		printf("mensagem> ");
		fgets(buf, BUFSZ-1, stdin);
		
		message = parse_message(buf);
		printf("message:%s\n",message);

		size_t count = send(s, message, strlen(message)+1, 0);
		if (count != strlen(message)+1) {
			exit(EXIT_FAILURE);
		}

		memset(buf, 0, BUFSZ);
		if(count != 0){	
			count = recv(s, buf, BUFSZ, 0);
			printf("%s\n", buf);
		}
		parse_return(buf);
	}



	close(s);
	exit(EXIT_SUCCESS);
}