/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>
#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 1024 // max number of bytes we can get at once 

#define TIMEOUT_MS 2000 // max milisseconds client will wait for server response

//=================== Send function ===========================
int send_message(int s, char *buf, int *len){
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0); // sends all bytes
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here
    printf("client: sent %s\n", buf);
    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

//=================== Receive function ===========================
char* receive_message(int numbytes, int sockfd, char* buf){
	struct pollfd pfds[1]; //Creates poll struct

	pfds[0].fd = sockfd; //Defines socket 
	pfds[0].events = POLLIN; //Define read event monitoring

	//Creates poll with timeout of 2000 ms
	int numevents = poll(pfds, 1, TIMEOUT_MS);

	//If timeout or error happens, exit the program
    if (numevents <= 0){
		printf("client: timeout limit exceeded\n");
		exit(1);
	} 

	//If data is received, use recv
	else { 
		if ((numbytes = recv(sockfd, buf, strlen(buf)-1, 0)) == -1) {
        	perror("recv");
        	exit(1);
    	}
		printf("client: received '%s'\n",buf);
	}
	return buf;
}

//=================== Receive Image Function =====================
void receive_image(int numbytes, int sockfd, char* name) {
    FILE *image;
    ssize_t recv_bytes;

	//Image struct defined
	typedef struct {
    	int totalPackets;
    	int currentPacket;
    	char imageData[MAXDATASIZE];
	} ImagePacket;

    ImagePacket packet;
    struct pollfd pfds[1]; //Creates poll struct

	pfds[0].fd = sockfd; //Defines socket 
	pfds[0].events = POLLIN; //Define read event monitoring

    // Define the count of packets received and the total expected
    int totalPacketsReceived = 1;
    int totalPacketsExpected = -1;
    int done = 0;

    while (!done) {
        pfds[0].fd = sockfd;
        pfds[0].events = POLLIN;

        //Creates poll with timeout of 2000 ms
        int numevents = poll(pfds, 1, TIMEOUT_MS);

		//if error happens, exit the program
        if (numevents < 0) {
            perror("client: error on image receive");
            exit(1);
        } 

		//If timeout happens, stop reading the image
		else if (numevents == 0) {
            printf("client: time limit exceeded\n");
			break;
        } 

		else {
            if (pfds[0].revents & POLLIN) {

                //If all packets were received, break
                if (totalPacketsReceived == totalPacketsExpected) {
                    done = 1;
                }

				//read the image struct
				if ((numbytes = recv(sockfd, &packet, sizeof(ImagePacket), 0)) == -1) {
        			perror("recv");
        			exit(1);
				}

				printf("client: received packet %i/%i\n", packet.currentPacket, packet.totalPackets);

                //If it is the first packet, define the expected packet number
                if (totalPacketsExpected == -1) {
                    totalPacketsExpected = packet.totalPackets;
					
                    //Create new file to save the image
					char nome_arquivo[120];
    				sprintf(nome_arquivo, "../download/%s.png", name);
                    image = fopen(nome_arquivo, "wb");
                    if (image == NULL) {
                        perror("server: image open error");
        				exit(1);
                    }
                }

				//Check if any package was lost
				if (totalPacketsReceived != packet.currentPacket){
					printf("client: some packets were lost\n");
					totalPacketsReceived == packet.currentPacket-1;
				}
                //Writes the received data 
                fwrite(packet.imageData, 1, sizeof(packet.imageData), image);
                totalPacketsReceived++;
            }
        }
    }

    //Close the image
    fclose(image);
    printf("client: image received\n");
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//===================== Main ================================
int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	//Start the client
	char entry[50];
	char out[5] = "exit";
	printf("client: Ola, seja bem-vindo, digite /help para ver os comandos\n");
	printf("client: Insira o comando: ");
	scanf("%s", &entry); //scans the command, i.e. "insert"
	while(strcmp(entry, out) != 0) { //while the string passed is not "exit" continue the program

		//if the user wants to see all the commands
		if (strcmp(entry, "/help") == 0) {
			printf("Comandos: \n");
			printf("\tinsert {dados}: inserir usuario novo\n");
			printf("\t\t{dados}: email;nome;sobrenome;residencia;formacaoacademica;anodeformatura;habilidades\n");
			printf("\tall : retornar todos os usuarios\n");
			printf("\temail : retornar um usuario especifico\n");
			printf("\tcourse : retornar todos os usuarios formados em um determinado curso\n");
			printf("\tskill : retornar todos os usuarios que tem uma determinada habilidade\n");
			printf("\tyear : retornar todos os usuarios formados em um determinado ano\n");
			printf("\tremove : remover usuario\n");
			printf("\tpicture : retorna a foto de perfil do usuario\n");
			printf("\texit : finalizar execucao\n");
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
			continue;
		}

		//insert method passed
		if(strcmp(entry, "insert")==0){
			char aux[MAXDATASIZE];
			fgets(aux, MAXDATASIZE, stdin); //gets the rest of the command, for example, the data from the insert
			aux[strlen(aux)-1] = '\0';
			strcat(entry, aux); //puts all the info in one string
			if(strlen(aux) == 0){
				printf("client: Comando inválido\n");
				printf("client: Insira o comando: ");
				scanf("%s", &entry);
				continue;
			}

			//sends the message to the client
			int len;
            len = strlen(entry);
            if (send_message(sockfd, entry, &len) == -1) {
                perror("send_message");
                printf("We only sent %d bytes because of the error!\n", len);
            } 

			//clears the buffer and waits for the response
			bzero(buf, MAXDATASIZE);
			receive_message(numbytes, sockfd, buf);
		
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
		}

		//get all method passed
		else if(strcmp(entry, "all")==0){

			//sends the message to the client
			int len;
            len = strlen(entry);
            if (send_message(sockfd, entry, &len) == -1) {
                perror("send_message");
                printf("We only sent %d bytes because of the error!\n", len);
            } 

			//clears the buffer and waits for the response
			bzero(buf, MAXDATASIZE);
			receive_message(numbytes, sockfd, buf);
		
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
		}

		//get by email method passed
		else if(strcmp(entry, "email")==0){
			char aux[MAXDATASIZE];
			fgets(aux, MAXDATASIZE, stdin); //gets the rest of the command, for example, the data from the insert
			aux[strlen(aux)-1] = '\0';
			strcat(entry, aux); //puts all the info in one string
			if(strlen(aux) == 0){
				printf("client: Comando inválido\n");
				printf("client: Insira o comando: ");
				scanf("%s", &entry);
				continue;
			}

			//sends the message to the client	
			int len;
            len = strlen(entry);
            if (send_message(sockfd, entry, &len) == -1) {
                perror("send_message");
                printf("We only sent %d bytes because of the error!\n", len);
            } 

			//clears the buffer and waits for the response
			bzero(buf, MAXDATASIZE);
			receive_message(numbytes, sockfd, buf);
		
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
		}

		//get by course method passed
		else if(strcmp(entry, "course")==0){
			char aux[MAXDATASIZE];
			fgets(aux, MAXDATASIZE, stdin); //gets the rest of the command, for example, the data from the insert
			aux[strlen(aux)-1] = '\0';
			strcat(entry, aux); //puts all the info in one string
			if(strlen(aux) == 0){
				printf("client: Comando inválido\n");
				printf("client: Insira o comando: ");
				scanf("%s", &entry);
				continue;
			}

			//sends the message to the client	
			int len;
            len = strlen(entry);
            if (send_message(sockfd, entry, &len) == -1) {
                perror("send_message");
                printf("We only sent %d bytes because of the error!\n", len);
            } 

			//clears the buffer and waits for the response
			bzero(buf, MAXDATASIZE);
			receive_message(numbytes, sockfd, buf);
		
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
		}

		//get by skill method passed
		else if(strcmp(entry, "skill")==0){
			char aux[MAXDATASIZE];
			fgets(aux, MAXDATASIZE, stdin); //gets the rest of the command, for example, the data from the insert
			aux[strlen(aux)-1] = '\0';
			strcat(entry, aux); //puts all the info in one string
			if(strlen(aux) == 0){
				printf("client: Comando inválido\n");
				printf("client: Insira o comando: ");
				scanf("%s", &entry);
				continue;
			}

			//sends the message to the client	
			int len;
            len = strlen(entry);
            if (send_message(sockfd, entry, &len) == -1) {
                perror("send_message");
                printf("We only sent %d bytes because of the error!\n", len);
            } 

			//clears the buffer and waits for the response
			bzero(buf, MAXDATASIZE);
			receive_message(numbytes, sockfd, buf);
		
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
		}

		//get by year method passed
		else if(strcmp(entry, "year")==0){
			char aux[MAXDATASIZE];
			fgets(aux, MAXDATASIZE, stdin); //gets the rest of the command, for example, the data from the insert
			aux[strlen(aux)-1] = '\0';
			strcat(entry, aux); //puts all the info in one string
			if(strlen(aux) == 0){
				printf("client: Comando inválido\n");
				printf("client: Insira o comando: ");
				scanf("%s", &entry);
				continue;
			}

			//sends the message to the client	
			int len;
            len = strlen(entry);
            if (send_message(sockfd, entry, &len) == -1) {
                perror("send_message");
                printf("We only sent %d bytes because of the error!\n", len);
            } 

			//clears the buffer and waits for the response
			bzero(buf, MAXDATASIZE);
			receive_message(numbytes, sockfd, buf);
		
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
		}

		//remove user method passed
		else if(strcmp(entry, "remove")==0){
			char aux[MAXDATASIZE];
			fgets(aux, MAXDATASIZE, stdin); //gets the rest of the command, for example, the data from the insert
			aux[strlen(aux)-1] = '\0';
			strcat(entry, aux); //puts all the info in one string
			if(strlen(aux) == 0){
				printf("client: Comando inválido\n");
				printf("client: Insira o comando: ");
				scanf("%s", &entry);
				continue;
			}

			//sends the message to the client	
			int len;
            len = strlen(entry);
            if (send_message(sockfd, entry, &len) == -1) {
                perror("send_message");
                printf("We only sent %d bytes because of the error!\n", len);
            } 

			//clears the buffer and waits for the response
			bzero(buf, MAXDATASIZE);
			receive_message(numbytes, sockfd, buf);
		
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
		}

		//picture method passed
		else if(strcmp(entry, "picture")==0){
			char aux[MAXDATASIZE];
			fgets(aux, MAXDATASIZE, stdin); //gets the rest of the command, for example, the data from the insert
			aux[strlen(aux)-1] = '\0';
			strcat(entry, aux); //puts all the info in one string
			if(strlen(aux) == 0){
				printf("client: Comando inválido\n");
				printf("client: Insira o comando: ");
				scanf("%s", &entry);
				continue;
			}

			//sends the message to the client	
			int len;
            len = strlen(entry);
            if (send_message(sockfd, entry, &len) == -1) {
                perror("send_message");
                printf("We only sent %d bytes because of the error!\n", len);
            } 

			//clears the buffer and waits for the response
			bzero(buf, MAXDATASIZE);
			receive_image(numbytes, sockfd, aux);
		
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
		}

		//if the command does not exist, repeat the request
		else{
			printf("client: Comando inválido\n");
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
		}
	}
	close(sockfd);

	return 0;
}
