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

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

void send_message(int numbytes, int sockfd, char buf[MAXDATASIZE]){

    if (send(sockfd, buf, strlen(buf), 0) == -1) {
        perror("send");
    }
    printf("client: sent '%s'\n", buf);
}

void receive_message(int numbytes, int sockfd, char buf[MAXDATASIZE]){
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
	printf("client: received '%s'\n",buf);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

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
	hints.ai_socktype = SOCK_STREAM;

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

	char entry[50];
	char out[5] = "exit";
	printf("client: Ola, seja bem-vindo, digite /help para ver os comandos\n");
	printf("client: Insira o comando: ");
	scanf("%s", &entry);
	while(strcmp(entry, out) != 0) {
		if (strcmp(entry, "/help") == 0) {
			printf("Comandos: \n");
			printf("\tinsert : inserir usuario novo\n");
			printf("\tall : retornar todos os usuarios\n");
			printf("\temail : retornar um usuario especifico\n");
			printf("\tcourse : retornar todos os usuarios formados em um determinado curso\n");
			printf("\tskill : retornar todos os usuarios que tem uma determinada habilidade\n");
			printf("\tyear : retornar todos os usuarios formados em um determinado ano\n");
			printf("\tremove : remover usuario\n");
			printf("\texit : finalizar execucao\n");
			printf("client: Insira o comando: ");
			scanf("%s", &entry);
			continue;
		}

		//================ SEND ==================

		send_message(numbytes, sockfd, entry);

		if(strcmp(entry,"insert") == 0){
			char aux[MAXDATASIZE];
			printf("client: Insira o email: ");
			scanf("%s", &aux);
			send_message(numbytes, sockfd, aux);
		}

		//============ RECEIVE ====================

		bzero(buf, MAXDATASIZE);
		receive_message(numbytes, sockfd, buf);
		
		printf("client: Insira o comando: ");
		scanf("%s", &entry);
	}
	close(sockfd);

	return 0;
}
