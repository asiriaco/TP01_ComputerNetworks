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
//METHOD TO EXTRACT INFO FROM FILE
char* extract_from_file(char *filename){
	if(filename[strlen(filename)-1] == '\n'){
		filename[strlen(filename)-1] = '\0';
	}
	FILE *fp;
	fp = fopen(filename, "r");
	if (fp == NULL){
		logexit("file not found");
	}

	char* result = (char*)malloc(1024 * sizeof(char));
	char line[1024];

	while(fgets(line, 1024, fp) != NULL){
		size_t len = strlen(line);
		if (line[len-1] == '\n'){
			line[len-1] = ' ';
		}
		strcat(result, line);
	}


	fclose(fp);
	return result;

}

char* parse_message(char *buf){

	//kill case
	if (strncmp(buf, "kill", 4) == 0){
		return "kill";
	}

    char* message = "";
	char *command;
    command = strtok(buf, " ");

	//REGISTER ROOM
    if (strcmp(command, "register") == 0){
		char* room_number = strtok(NULL, " ");
		int number = atoi(room_number);
		if(number > MAX_ROOM || number < 0){
			logerror(01);
			message = "ERROR";
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
		//CLI
		if(strcmp(init_command, "info") == 0){
			info = strtok(NULL, "\0");
			strcpy(aux, info);
		}
		//FILE
		else if (strcmp(init_command, "file") == 0){
			char* file = strtok(NULL, "\0");
			aux = extract_from_file(file);
			strcpy(info, aux);
		}
		else{
			exit(EXIT_FAILURE);
		}
		if (is_info_valid(info)){
			char init_message[1024];
			sprintf(init_message, "INI_REQ %s", aux);
			message = init_message;
		}
		else{
			message = "ERROR";
		}
		
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
		//CLI
		if(strcmp(init_command, "info") == 0){
			info = strtok(NULL, "\0");
			strcpy(aux, info);
		}
		//FILE
		else if (strcmp(init_command, "file") == 0){
			char* file = strtok(NULL, "\0");
			aux = extract_from_file(file);
			strcpy(info, aux);
		}
		else{
			exit(EXIT_FAILURE);
		}
		if (is_info_valid(info)){
			char update_message[1024];
			sprintf(update_message, "ALT_REQ %s", aux);
			message = update_message;
		}
    }

	//LOAD INFO FROM A SINGLE ROOM OR ALL ROOMS
    else if (strcmp(command, "load")== 0){
		char * opmode = strtok(NULL, " ");
		//SINGLE ROOM
		if(strcmp(opmode, "info") == 0){
			char load_message[1024];
			char* room_number = strtok(NULL, " ");
			int number = atoi(room_number);
			sprintf(load_message, "LOAD_REQ %d", number);
			message = load_message;
		}
		//ALL ROOMS
		else if (strncmp(opmode, "rooms", 5) == 0){
			message = "INF_REQ";
		}
		else{
			exit(EXIT_FAILURE);
		}
    }

    else{
        exit(EXIT_FAILURE);
	}
	return message;
}

void parse_return(char *buf){

	if (strncmp(buf, "OK", 2) == 0){
		char* message = strtok(buf, " ");
		message = strtok(NULL, " ");
		logok(atoi(message));
	}
	else if (strncmp(buf, "ERROR", 5) == 0){
		char* message = strtok(buf, " ");
		message = strtok(NULL, " ");
		logerror(atoi(message));
	}
	else {
		printf("%s", buf);
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

		if(strncmp(message, "ERROR", 5) == 0){
			continue;
		}

		size_t count = send(s, message, strlen(message)+1, 0);
		if (count != strlen(message)+1) {
			exit(EXIT_FAILURE);
		}

		if (strncmp(message, "kill", 4) == 0){
			close(s);
			exit(EXIT_SUCCESS);
        }

		memset(buf, 0, BUFSZ);
		if(count != 0){	
			count = recv(s, buf, BUFSZ, 0);
		}
		parse_return(buf);
	}
	close(s);
	exit(EXIT_SUCCESS);
}