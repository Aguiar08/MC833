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

int save_data(char* dados) {
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
    if(i<6){
        printf("server: Missing data on insert\n");
        return 0;
    }
    
    char nome_arquivo[109];
    sprintf(nome_arquivo, "../users/%s.txt", email);
    arquivo = fopen(nome_arquivo, "a+");
    if(arquivo == NULL) {
        printf("server: Open file error\n");
        return 0;
    }
    
    fprintf(arquivo, "Email: %s\n", email);
    fprintf(arquivo, "Nome: %s Sobrenome: %s\n", nome, sobrenome);
    fprintf(arquivo, "Residência: %s\n", residencia);
    fprintf(arquivo, "Formação Acadêmica: %s\n", formacao);
    fprintf(arquivo, "Ano de Formatura: %s\n", ano);
    fprintf(arquivo, "Habilidades: %s\n", habilidades);

    fclose(arquivo);
    return 1;
}

void send_message(int new_fd, char buf[MAXDATASIZE]){

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

int get_profile(char* email, int new_fd) {
    char nome_arquivo[109];
    sprintf(nome_arquivo, "../users/%s.txt", email);
    FILE *file = fopen(nome_arquivo, "r");
    if (file == NULL){
        printf("server: open file error!\n");
        return 0;
    }

    // char[6][] carac = {"Email", "Nome", "Residencia", "Formacao Academica", "Ano de Formatura", "Habilidades"}

    char linha[MAXDATASIZE];
    char result[MAXDATASIZE];
    bzero(result, MAXDATASIZE);
    int i = 0;
    while (!feof(file)){
        fgets(linha, MAXDATASIZE, file);
        if(i<6) strcat(result, linha);
        i++;
    }
    fclose(file);
    send_message(new_fd, result);
    return 1;
}

int get_profile_by_course(char* course, int new_fd) {
    DIR *folder;
    struct dirent *entry;

    folder = opendir("../users/.");
    if (folder == NULL) {
        printf("server: open folder error!\n");
        return 0;
    }

    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {
            char nome_arquivo[109];
            sprintf(nome_arquivo, "../users/%s", entry->d_name);
            FILE *file = fopen(nome_arquivo, "r");
            if (file == NULL) {
                printf("server: open file error!\n");
                continue;
            }

            // ler o arquivo de texto
            char buffer[MAXDATASIZE];
            char *curso = NULL;
            char result[MAXDATASIZE];
            while (fgets(buffer, MAXDATASIZE, file) != NULL) {
                if (strstr(buffer, "Formação Acadêmica") != NULL) {
                    curso = strchr(buffer, ':') + 2;
                    curso[strlen(curso)-1] = '\0';
                } else if (strstr(buffer, "Email") != NULL) {
                    strcpy(result, buffer);
                } else if (strstr(buffer, "Nome") != NULL) {
                    strcat(result, buffer);
                }
                
                if (curso != NULL && strcmp(curso, course) == 0) {
                    send_message(new_fd, result);
                }
            }

            fclose(file);
        }
    }

    closedir(folder);
    send_message(new_fd, "end");
    return 1;
}

void get_profile_by_ability() {

}

int get_profile_by_year(char* year, int new_fd) {
    DIR *folder;
    struct dirent *entry;

    folder = opendir("../users/.");
    if (folder == NULL) {
        printf("server: Open folder error\n");
        return 0;
    }

    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {
            char nome_arquivo[109];
            sprintf(nome_arquivo, "../users/%s", entry->d_name);
            FILE *file = fopen(nome_arquivo, "r");
            if (file == NULL) {
                printf("server: Open file error\n");
                printf("server: %s\n", nome_arquivo);
                continue;
            }

            // ler o arquivo de texto
            char buffer[MAXDATASIZE];
            char output[MAXDATASIZE];
            char *ano = NULL;
            while (fgets(buffer, MAXDATASIZE, file) != NULL) {
                if (strstr(buffer, "Email") != NULL) {
                    strcpy(output,buffer);
                }
                else if (strstr(buffer, "Nome") != NULL) {
                    strcat(output,buffer);
                }
                else if (strstr(buffer, "Formação Acadêmica") != NULL) {
                    strcat(output,buffer);
                }
                else if (strstr(buffer, "Ano de Formatura") != NULL) {
                    ano = strchr(buffer, ':') + 2;
                    ano[strlen(ano)-1] = '\0';
                }

                if (ano != NULL && strcmp(ano, year) == 0) {
                    send_message(new_fd, output);
                }
            }

            fclose(file);
        }
    }

    closedir(folder);
    send_message(new_fd, "end");
    return 1;
}

int get_all() {
    DIR *folder;
    struct dirent *entry;

    folder = opendir("../users/.");
    if (folder == NULL) {
        printf("server: unable to open folder with user files!\n");
        return 0;
    }

    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {
            char nome_arquivo[109];
            sprintf(nome_arquivo, "../users/%s", entry->d_name);
            FILE *file = fopen(nome_arquivo, "r");
            if (file == NULL) {
                printf("server: open file error!\n");
                continue;
            }

            char buffer[MAXDATASIZE];
            while (fgets(buffer, MAXDATASIZE, file) != NULL)
                printf("%s", buffer);

            fclose(file);
        }
    }

    closedir(folder);
    return 1;
}

int remove_profile(char* email) {
    char nome_arquivo[109];
    sprintf(nome_arquivo, "../users/%s.txt", email);
    FILE *file = fopen(nome_arquivo, "r");
    if (file == NULL){
        printf("server: File not found\n");
        return 0;
    }
    fclose(file);
    if (remove(nome_arquivo) == 0) {
        return 1;
    } else {
        printf("server: File remove error\n");
        return 0;
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

    while(1) {  // main accept() loop

        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener
            while(1){

                //========== RECEIVE ===================
                receive_message(numbytes, new_fd, buf);

                if(strcmp(buf, "insert") == 0){
                    char entry[MAXDATASIZE];
                    bzero(entry, MAXDATASIZE);
                    if(save_data(receive_message(numbytes, new_fd, entry))){
                        printf("server: Client saved in the database\n");
                        send_message(new_fd, "Operation Successful");
                    }
                    else{
                        send_message(new_fd, "Operation Failed"); 
                    }
                    
	            }
                else if(strcmp(buf, "all") == 0){
			        get_all();
                }
                else if(strcmp(buf, "email") == 0){
                    char entry[MAXDATASIZE];
                    bzero(entry, MAXDATASIZE);
                    if(!get_profile(receive_message(numbytes, new_fd, entry), new_fd)){
                        send_message(new_fd, "Operation Failed"); 
                    }
                }
                else if(strcmp(buf, "course") == 0){
                    char entry[MAXDATASIZE];
                    bzero(entry, MAXDATASIZE);
                    if(!get_profile_by_course(receive_message(numbytes, new_fd, entry), new_fd)){
                        send_message(new_fd, "Operation Failed"); 
                    }
                }
                else if(strcmp(buf, "skill") == 0){
                    
                }
                else if(strcmp(buf, "year") == 0){
                    char entry[MAXDATASIZE];
                    bzero(entry, MAXDATASIZE);
                    if(!get_profile_by_year(receive_message(numbytes, new_fd, entry), new_fd)){
                        send_message(new_fd, "Operation Failed"); 
                    }
                }
                else if(strcmp(buf, "remove") == 0){
                    char entry[MAXDATASIZE];
                    bzero(entry, MAXDATASIZE);
                    if(remove_profile(receive_message(numbytes, new_fd, entry))){
                        printf("server: Client removed from the database\n");
                        send_message(new_fd, "Operation Successful");
                    }
                    else{
                        send_message(new_fd, "Operation Failed"); 
                    }
                }
                else if(strcmp(buf, "") == 0){
                    printf("server: Exiting connection from %s\n", s);
                    break;
	            }
                else{
                    //========== SEND ===================
                    send_message(new_fd, "error");
                }
                bzero(buf, MAXDATASIZE);
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    }

    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
        perror("accept");
        exit(1);
    }

    // while(1) {  // main accept() loop

    //     //========== RECEIVE ===================
    //     bzero(buf, MAXDATASIZE);
    //     receive_message(numbytes, new_fd, buf);
    
    //     if(strcmp(buf, "insert") == 0){
    //         char entry[MAXDATASIZE];
    //         save_data(receive_message(numbytes, new_fd, entry));
    //         char * buf = "server: Client saved in the database";
	//     }

    //     //========== SEND ===================

    //     send_message(numbytes, new_fd, buf);
    // }
    // close(sockfd);

    return 0;
}
