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
    char email[100], nome[100], sobrenome[100], residencia[100], formacao[100], ano[5], habilidades[500];
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
                return 0;
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
    arquivo = fopen(nome_arquivo, "r");
    if (arquivo != NULL){
        printf("server: This user already exists!\n");
        fclose(arquivo);
        return 0;
    }
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

int send_message(int s, char *buf, int *len){
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here
    printf("server: sent %s\n", buf);
    return n==-1?-1:0; // return -1 on failure, 0 on success
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
    int len;
    len = strlen(result);
    if (send_message(new_fd, result, &len) == -1) {
        perror("send_message");
        printf("We only sent %d bytes because of the error!\n", len);
    } 
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

    char * final = (char *) malloc(1);
    *final = '\0'; 
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
            char result [MAXDATASIZE];
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
                    final = (char *)realloc(final, strlen(final)+strlen(result)+1);
                    strcat(final,result);
                }
            }

            fclose(file);
        }
    }

    closedir(folder);
    int len;
    len = strlen(final);
    if (send_message(new_fd, final, &len) == -1) {
        perror("send_message");
        printf("We only sent %d bytes because of the error!\n", len);
    } 
    free(final);
    return 1;
}

int get_profile_by_skill(char* skill, int new_fd) {
    DIR *folder;
    struct dirent *entry;

    folder = opendir("../users/.");
    if (folder == NULL){
        printf("server: Open folder error\n");
        return 0;
    }

    char * final = (char *) malloc(1);
    *final = '\0'; 
    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {
            char nome_arquivo[109];
            sprintf(nome_arquivo, "../users/%s", entry->d_name);
            FILE *file = fopen(nome_arquivo, "r");
            if (file == NULL) {
                printf("server: open file error\n");
                printf("server: %s\n", nome_arquivo);
                continue;
            }

            char buffer[MAXDATASIZE];
            char output[MAXDATASIZE];
            while (fgets(buffer, MAXDATASIZE, file) != NULL) {
                if (strstr(buffer, "Email") != NULL) {
                    strcpy(output,buffer);
                }
                else if (strstr(buffer, "Nome") != NULL) {
                    strcat(output,buffer);
                }
                else if (strstr(buffer, "Habilidades") != NULL) {
                    if (strstr(buffer, skill) != NULL) {
                        final = (char *)realloc(final, strlen(final)+strlen(output)+1);
                        strcat(final,output);
                    }
                }
            }

            fclose(file);
        }
    }

    closedir(folder);
    int len;
    len = strlen(final);
    if (send_message(new_fd, final, &len) == -1) {
        perror("send_message");
        printf("We only sent %d bytes because of the error!\n", len);
    } 
    free(final);
    return 1;
}

int get_profile_by_year(char* year, int new_fd) {
    DIR *folder;
    struct dirent *entry;

    folder = opendir("../users/.");
    if (folder == NULL) {
        printf("server: Open folder error\n");
        return 0;
    }

    char * final = (char *) malloc(1);
    *final = '\0'; 
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
                    final = (char *)realloc(final, strlen(final)+strlen(output)+1);
                    strcat(final,output);
                }
            }

            fclose(file);
        }
    }

    closedir(folder);
    int len;
    len = strlen(final);
    if (send_message(new_fd, final, &len) == -1) {
        perror("send_message");
        printf("We only sent %d bytes because of the error!\n", len);
    } 
    free(final);
    return 1;
}

int get_all(int new_fd) {
    DIR *folder;
    struct dirent *entry;

    folder = opendir("../users/.");
    if (folder == NULL) {
        printf("server: Open folder error\n");
        return 0;
    }
    char * final = (char *) malloc(1);
    *final = '\0'; 
    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {
            char nome_arquivo[109];
            sprintf(nome_arquivo, "../users/%s", entry->d_name);
            FILE *file = fopen(nome_arquivo, "r");
            if (file == NULL) {
                printf("server: open file error\n");
                continue;
            }

            char buffer[MAXDATASIZE];
            char output[MAXDATASIZE];
            while (fgets(buffer, MAXDATASIZE, file) != NULL)
                if (strstr(buffer, "Email") != NULL) {
                    strcpy(output,buffer);
                } else {
                    strcat(output, buffer);
                }

            final = (char *)realloc(final, strlen(final)+strlen(output)+1);
            strcat(final,output);

            fclose(file);
        }
    }

    closedir(folder);
    int len;
    len = strlen(final);
    if (send_message(new_fd, final, &len) == -1) {
        perror("send_message");
        printf("We only sent %d bytes because of the error!\n", len);
    } 
    free(final);
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
                bzero(buf, MAXDATASIZE);
                receive_message(numbytes, new_fd, buf);
                char first = buf[0];
                if(first == 'i'){
                    memmove(buf, buf+7, strlen(buf));
                    if(save_data(buf)){
                        printf("server: Client saved in the database\n");
                        int len;
                        len = strlen("Operation Successful");
                        if (send_message(new_fd, "Operation Successful", &len) == -1) {
                            perror("send_message");
                            printf("We only sent %d bytes because of the error!\n", len);
                        } 
                    }
                    else{
                        int len;
                        len = strlen("Operation Failed");
                        if (send_message(new_fd, "Operation Failed", &len) == -1) {
                            perror("send_message");
                            printf("We only sent %d bytes because of the error!\n", len);
                        }
                    }
                    
	            }
                else if(first == 'a'){
                    if (!get_all(new_fd)){
                        int len;
                        len = strlen("Operation Failed");
                        if (send_message(new_fd, "Operation Failed", &len) == -1) {
                            perror("send_message");
                            printf("We only sent %d bytes because of the error!\n", len);
                        }
                    }
                }
                else if(first == 'e'){
                    memmove(buf, buf+6, strlen(buf));
                    if(!get_profile(buf, new_fd)){
                        int len;
                        len = strlen("Operation Failed");
                        if (send_message(new_fd, "Operation Failed", &len) == -1) {
                            perror("send_message");
                            printf("We only sent %d bytes because of the error!\n", len);
                        } 
                    }
                }
                else if(first == 'c'){
                    memmove(buf, buf+7, strlen(buf));
                    if(!get_profile_by_course(buf, new_fd)){
                        int len;
                        len = strlen("Operation Failed");
                        if (send_message(new_fd, "Operation Failed", &len) == -1) {
                            perror("send_message");
                            printf("We only sent %d bytes because of the error!\n", len);
                        }
                    }
                }
                else if(first == 's'){
                    memmove(buf, buf+6, strlen(buf));
                    if(!get_profile_by_skill(buf, new_fd)){
                        int len;
                        len = strlen("Operation Failed");
                        if (send_message(new_fd, "Operation Failed", &len) == -1) {
                            perror("send_message");
                            printf("We only sent %d bytes because of the error!\n", len);
                        } 
                    }
                }
                else if(first == 'y'){
                    memmove(buf, buf+5, strlen(buf));
                    if(!get_profile_by_year(buf, new_fd)){
                        int len;
                        len = strlen("Operation Failed");
                        if (send_message(new_fd, "Operation Failed", &len) == -1) {
                            perror("send_message");
                            printf("We only sent %d bytes because of the error!\n", len);
                        }
                    }
                }
                else if(first == 'r'){
                    memmove(buf, buf+7, strlen(buf));
                    if(remove_profile(buf)){
                        printf("server: Client removed from the database\n");
                        int len;
                        len = strlen("Operation Successful");
                        if (send_message(new_fd, "Operation Successful", &len) == -1) {
                            perror("send_message");
                            printf("We only sent %d bytes because of the error!\n", len);
                        }
                    }
                    else{
                        int len;
                        len = strlen("Operation Failed");
                        if (send_message(new_fd, "Operation Failed", &len) == -1) {
                            perror("send_message");
                            printf("We only sent %d bytes because of the error!\n", len);
                        } 
                    }
                }
                else if(strcmp(buf, "") == 0){
                    printf("server: Exiting connection from %s\n", s);
                    break;
	            }
                else{
                    //========== SEND ===================
                    int len;
                    len = strlen("error");
                    if (send_message(new_fd, "error", &len) == -1) {
                        perror("send_message");
                        printf("We only sent %d bytes because of the error!\n", len);
                    }
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
