
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

#define MAXDATASIZE 100 // max number of bytes we can get at once 


void send_message(int numbytes, int new_fd, char buf[MAXDATASIZE]){

    if (send(new_fd, buf, strlen(buf), 0) == -1) {
        perror("send");
    }
    printf("server: sent '%s'\n", buf);
}

void receive_message(int numbytes, int new_fd, char buf[MAXDATASIZE]){
    if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
	printf("server: received '%s'\n",buf);
}

void create_profile(int numbytes, int new_fd) {
    
    char email[50], nome[50], sobrenome[50], residencia[50], formacao[50], habilidades[100];
    int ano_formatura;
    printf("Digite o seu email: ");
    if ((numbytes = recv(new_fd, email, MAXDATASIZE-1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

	email[numbytes] = '\0';
    printf("%s", email);
    return 0;
    printf("Digite o seu nome: ");
    scanf("%s", nome);
    printf("Digite o seu sobrenome: ");
    scanf("%s", sobrenome);
    printf("Digite a sua residencia: ");
    scanf("%s", residencia);
    printf("Digite a sua formacao academica: ");
    scanf("%s", formacao);
    printf("Digite o ano de sua formatura: ");
    scanf("%d", &ano_formatura);
    printf("Digite suas habilidades: ");
    fgets(habilidades, 100, stdin);
    FILE *file = fopen(email, "w");
    if (file == NULL) {
        printf("Erro ao criar arquivo!\n");
        return 1;
    }
    fprintf(file, "Email: %s\n", email);
    fprintf(file, "Nome: %s Sobrenome: %s\n", nome, sobrenome);
    fprintf(file, "Residencia: %s\n", residencia);
    fprintf(file, "Formacao Academica: %s\n", formacao);
    fprintf(file, "Ano de Formatura: %d\n", ano_formatura);
    fprintf(file, "Habilidades: %s\n", habilidades);
    fclose(file);
    printf("server: Perfil criado e salvo em perfil.txt com sucesso!\n");
    return "Perfil criado e salvo em perfil.txt com sucesso!";
}

void get_profile() {

}

void get_profile_by_course() {

}

void get_profile_by_ability() {

}

void get_profile_by_year() {

}

void get_all() {

}

void remove_profile() {

}

void sigchld_handler(int s){ 
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    int numbytes;  
	char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        exit(1);
    }

    while(1) {  // main accept() loop

        //========== RECEIVE ===================
        bzero(buf, MAXDATASIZE);
        receive_message(numbytes, new_fd, buf);
    
        if(strcmp(buf, "insert") == 0){
            char email[50], nome[50], sobrenome[50], residencia[50], formacao[50], habilidades[100];
            int ano_formatura;
            receive_message(numbytes, new_fd, email);
	    }

        //========== SEND ===================

        send_message(numbytes, new_fd, buf);
    }
    close(sockfd);

    return 0;
}
