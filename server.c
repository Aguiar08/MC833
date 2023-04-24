
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
#include <dirent.h>

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

#define MAXDATASIZE 1024 // max number of bytes we can get at once 

void save_data(char* dados) {
    FILE* arquivo;
    char* campo;
    char email[100], nome[100], sobrenome[100], residencia[100], formacao[100], ano[5], habilidades[200];

    campo = strtok(dados, ";");
    int i = 0;
    while(campo != NULL) {
        switch (i) {
            case 0:
                strcpy(email, campo);
                break;
            case 1:
                strcpy(nome, campo);
                break;
            case 2:
                strcpy(sobrenome, campo);
                break;
            case 3:
                strcpy(residencia, campo);
                break;
            case 4:
                strcpy(formacao, campo);
                break;
            case 5:
                strcpy(ano, campo);
                break;
            case 6:
                strcpy(habilidades, campo);
                break;
            default:
                printf("Dados inválidos\n");
                return;
        }
        campo = strtok(NULL, ";");
        i++;
    }
    char nome_arquivo[100];
    sprintf(nome_arquivo, "../users/%s.txt", email);
    arquivo = fopen(nome_arquivo, "a+");
    if(arquivo == NULL) {
        printf("Erro ao abrir o arquivo\n");
        return;
    }
    
    fprintf(arquivo, "Email: %s\n", email);
    fprintf(arquivo, "Nome: %s Sobrenome: %s\n", nome, sobrenome);
    fprintf(arquivo, "Residência: %s\n", residencia);
    fprintf(arquivo, "Formação Acadêmica: %s\n", formacao);
    fprintf(arquivo, "Ano de Formatura: %s\n", ano);
    fprintf(arquivo, "Habilidades: %s\n", habilidades);

    fclose(arquivo);
}

void send_message(int numbytes, int new_fd, char buf[MAXDATASIZE]){

    if (send(new_fd, buf, strlen(buf), 0) == -1) {
        perror("send");
    }
    printf("server: sent '%s'\n", buf);
}

char* receive_message(int numbytes, int new_fd, char buf[MAXDATASIZE]){
    if ((numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
	printf("server: received '%s'\n",buf);
    return buf;
}

void get_profile(char* email) {
    FILE *file = fopen(email, "r");
    if (file == NULL){
        printf("Arquivo não encontrado!\n");
        return;
    }

    // char[6][] carac = {"Email", "Nome", "Residencia", "Formacao Academica", "Ano de Formatura", "Habilidades"}

    char linha[100];
    char *result;
    while (!feof(file)){
        result = fgets(linha, 100, file);
        if (result)
            printf("%s", result);
        else {
            printf("Erro ao ler arquivo!\n");
            return;
        }
    }
    fclose(file);
}

void get_profile_by_course(char* course) {
    DIR *folder;
    struct dirent *entry;

    folder = opendir(".");
    if (folder == NULL) {
        perror("Erro ao abrir pasta com os arquivos dos usuarios!\n");
        return;
    }

    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {
            FILE *file = fopen(entry->d_name, "r");
            if (file == NULL) {
                perror("Nao foi possivel abrir o arquivo!\n");
                continue;
            }

            // ler o arquivo de texto
            char buffer[1000];
            char *curso = NULL;
            while (fgets(buffer, 1000, file) != NULL) {
                if (strstr(buffer, "Formacao Academica") != NULL) {
                    curso = strchr(buffer, ':') + 2;
                    curso[strlen(curso)-2] = '\0';
                }

                if (curso != NULL && strcmp(curso, course) == 0) {
                    printf("%s", buffer);
                }
            }

            fclose(file);
        }
    }

    closedir(folder);
    return;
}

void get_profile_by_ability() {

}

void get_profile_by_year(int year) {
    DIR *folder;
    struct dirent *entry;

    folder = opendir(".");
    if (folder == NULL) {
        perror("Erro ao abrir pasta com os arquivos dos usuarios!\n");
        return;
    }

    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {
            FILE *file = fopen(entry->d_name, "r");
            if (file == NULL) {
                perror("Nao foi possivel abrir o arquivo!\n");
                continue;
            }

            // ler o arquivo de texto
            char buffer[1000];
            char *ano = NULL;
            while (fgets(buffer, 1000, file) != NULL) {
                if (strstr(buffer, "Ano de Formatura") != NULL) {
                    ano = strchr(buffer, ':') + 2;
                    ano[strlen(ano)-2] = '\0';
                }

                if (ano != NULL && strcmp(ano, year) == 0) {
                    printf("%s", buffer);
                }
            }

            fclose(file);
        }
    }

    closedir(folder);
    return;
}

void get_all() {
    DIR *folder;
    struct dirent *entry;

    folder = opendir(".");
    if (folder == NULL) {
        perror("Erro ao abrir pasta com os arquivos dos usuarios!\n");
        return;
    }

    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {
            FILE *file = fopen(entry->d_name, "r");
            if (file == NULL) {
                perror("Nao foi possivel abrir o arquivo!\n");
                continue;
            }

            char buffer[1000];
            while (fgets(buffer, 1000, file) != NULL) {
                printf("%s", buffer);
            }

            fclose(file);
        }
    }

    closedir(folder);
    return;
}

void remove_profile(char* email) {
    FILE *file = fopen(email, "r");
    if (file == NULL){
        printf("Arquivo não encontrado!\n");
        return;
    }

    if (remove(email) == 0) {
        printf("Arquivo %s removido com sucesso!\n", email);
    } else {
        printf("Erro ao remover arquivo %s!\n", email);
    }
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
            char entry[MAXDATASIZE];
            save_data(receive_message(numbytes, new_fd, entry));
            char * buf = "server: Client saved in the database";
	    }

        //========== SEND ===================

        send_message(numbytes, new_fd, buf);
    }
    close(sockfd);

    return 0;
}