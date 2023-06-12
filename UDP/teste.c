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

//===================== Save Data ================================
int save_data(char* dados) {
    FILE* arquivo; //declare file archive
    char* campo;
    char email[100], nome[100], sobrenome[100], residencia[100], formacao[100], ano[5], habilidades[500]; //declare size of the user information
    campo = strtok(dados, ";"); //separate information based on the ; string
    int i = 0;
    while(campo != NULL) {
        switch (i) {
            case 0:
                strcpy(email, campo); //first ocurrance is the email
                break;
            case 1:
                strcpy(nome, campo); //second ocurrance is the name
                break;
            case 2:
                strcpy(sobrenome, campo); //third ocurrance is the surname
                break;
            case 3:
                strcpy(residencia, campo); //fourth ocurrance is the address
                break;
            case 4:
                strcpy(formacao, campo); //this ocurrance is the degree
                break;
            case 5:
                strcpy(ano, campo); //this ocurrance is the year of graduation
                break;
            case 6:
                strcpy(habilidades, campo); //this is the skill field
                break;
            default:
                printf("Dados inválidos\n"); //if there is more information than necessary
                return 0;
        }
        campo = strtok(NULL, ";");
        i++;
    }
    if(i<6){
        printf("server: Missing data on insert\n"); //if there is less information than the necessary returns error
        return 0;
    }
    
    //checking if the file already exists
    char nome_arquivo[109];
    sprintf(nome_arquivo, "../users/%s.txt", email);
    arquivo = fopen(nome_arquivo, "r"); 
    if (arquivo != NULL){
        printf("server: This user already exists!\n"); //If exists, avoid inserting again
        fclose(arquivo);
        return 0;
    }
    //if does not exist, add a new file
    arquivo = fopen(nome_arquivo, "a+");
    if(arquivo == NULL) {
        printf("server: Open file error\n");
        return 0;
    }
    
    //save data in the correct format
    fprintf(arquivo, "Email: %s\n", email);
    fprintf(arquivo, "Nome: %s Sobrenome: %s\n", nome, sobrenome);
    fprintf(arquivo, "Residência: %s\n", residencia);
    fprintf(arquivo, "Formação Acadêmica: %s\n", formacao);
    fprintf(arquivo, "Ano de Formatura: %s\n", ano);
    fprintf(arquivo, "Habilidades: %s\n", habilidades);

    fclose(arquivo);
    return 1;
}


//===================== Send Message ================================
int send_message(int sockfd, char *buf, int *len, struct sockaddr_storage their_addr, socklen_t sin_size){
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
    while(total < *len) {
        n = sendto(sockfd, buf+total, bytesleft, 0,(struct sockaddr *) &their_addr, sin_size);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here
    printf("server: sent %s\n", buf);
    return n==-1?-1:0; // return -1 on failure, 0 on success
} 

//===================== Get profile by Email ================================
int get_profile(char* email, int new_fd, struct sockaddr_storage their_addr, socklen_t sin_size) {
    char nome_arquivo[109];
    sprintf(nome_arquivo, "../users/%s.txt", email); //add the path to the file requested

    //checks if the file exists
    FILE *file = fopen(nome_arquivo, "r");
    if (file == NULL){
        printf("server: open file error!\n"); //return error if it does not exist
        return 0; 
    }

    //iterate throuhg the file adding all the information
    char linha[MAXDATASIZE];
    char result[MAXDATASIZE];
    bzero(result, MAXDATASIZE);
    int i = 0;
    while (!feof(file)){
        fgets(linha, MAXDATASIZE, file);
        if(i<6) strcat(result, linha); //adds the line to the string to be send
        i++;
    }
    fclose(file);

    //sends the information
    int len;
    len = strlen(result);
    if (send_message(new_fd, result, &len, their_addr, sin_size) == -1) {
        perror("send_message");
        printf("We only sent %d bytes because of the error!\n", len);
    } 
    return 1;
}

//===================== Get profile by Course ================================
int get_profile_by_course(char* course, int new_fd, struct sockaddr_storage their_addr, socklen_t sin_size) {
    DIR *folder;
    struct dirent *entry;

    //Tries to open the users directory
    folder = opendir("../users/.");
    if (folder == NULL) {
        printf("server: open folder error!\n"); //If it fails, return error
        return 0;
    }

    //Allocates message sent dize
    char * final = (char *) malloc(1);
    *final = '\0'; 

    //Iterates through all the files
    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {

            //Open the file
            char nome_arquivo[109];
            sprintf(nome_arquivo, "../users/%s", entry->d_name);
            FILE *file = fopen(nome_arquivo, "r");
            if (file == NULL) {
                printf("server: open file error!\n"); //If it fails, jump to next file
                continue;
            }

            //Read the file
            char buffer[MAXDATASIZE];
            char *curso = NULL;
            char result [MAXDATASIZE];
            while (fgets(buffer, MAXDATASIZE, file) != NULL) {

                //Save user course information
                if (strstr(buffer, "Formação Acadêmica") != NULL) {
                    curso = strchr(buffer, ':') + 2;
                    curso[strlen(curso)-1] = '\0';
                
                //Append Email info
                } else if (strstr(buffer, "Email") != NULL) {
                    strcpy(result, buffer);

                //Append Name info
                } else if (strstr(buffer, "Nome") != NULL) {
                    strcat(result, buffer);
                }
                
                //If the course info is equals to the passed one
                if (curso != NULL && strcmp(curso, course) == 0) {
                    //Realloc the message and add the information of the user
                    final = (char *)realloc(final, strlen(final)+strlen(result)+1);
                    strcat(final,result);
                }
            }

            fclose(file);
        }
    }

    //Send the final message
    closedir(folder);
    int len;
    len = strlen(final);
    if (send_message(new_fd, final, &len, their_addr, sin_size) == -1) {
        perror("send_message");
        printf("We only sent %d bytes because of the error!\n", len);
    } 
    free(final); //Free the memory
    return 1;
}

//===================== Get profile by Skill ================================
int get_profile_by_skill(char* skill, int new_fd, struct sockaddr_storage their_addr, socklen_t sin_size) {
    DIR *folder;
    struct dirent *entry;

    //Tries to open the users directory
    folder = opendir("../users/.");
    if (folder == NULL) {
        printf("server: open folder error!\n"); //If it fails, return error
        return 0;
    }

    //Allocates message sent dize
    char * final = (char *) malloc(1);
    *final = '\0';  

    //Iterates through all the files
    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {

            //Open the file
            char nome_arquivo[109];
            sprintf(nome_arquivo, "../users/%s", entry->d_name);
            FILE *file = fopen(nome_arquivo, "r");
            if (file == NULL) {
                printf("server: open file error!\n"); //If it fails, jump to next file
                printf("server: %s\n", nome_arquivo);
                continue;
            }

            //Read the file
            char buffer[MAXDATASIZE];
            char output[MAXDATASIZE];
            while (fgets(buffer, MAXDATASIZE, file) != NULL) {

                //Append Email info
                if (strstr(buffer, "Email") != NULL) {
                    strcpy(output,buffer);
                }

                //Append Name info
                else if (strstr(buffer, "Nome") != NULL) {
                    strcat(output,buffer);
                }

                //If passed skill is present in the current user
                else if (strstr(buffer, "Habilidades") != NULL) {
                    //Add the information to the final message
                    if (strstr(buffer, skill) != NULL) {
                        //Realloc the message and add the information of the user
                        final = (char *)realloc(final, strlen(final)+strlen(output)+1);
                        strcat(final,output);
                    }
                }
            }

            fclose(file);
        }
    }

    //Send the final message
    closedir(folder);
    int len;
    len = strlen(final);
    if (send_message(new_fd, final, &len, their_addr, sin_size) == -1) {
        perror("send_message");
        printf("We only sent %d bytes because of the error!\n", len);
    } 
    free(final); //Free memory
    return 1;
}

//===================== Get profile by Year ================================
int get_profile_by_year(char* year, int new_fd, struct sockaddr_storage their_addr, socklen_t sin_size) {
    DIR *folder;
    struct dirent *entry;

    //Tries to open the users directory
    folder = opendir("../users/.");
    if (folder == NULL) {
        printf("server: open folder error!\n"); //If it fails, return error
        return 0;
    }

    //Allocates message sent dize
    char * final = (char *) malloc(1);
    *final = '\0';  

    //Iterates through all the files
    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {

            //Open the file
            char nome_arquivo[109];
            sprintf(nome_arquivo, "../users/%s", entry->d_name);
            FILE *file = fopen(nome_arquivo, "r");
            if (file == NULL) {
                printf("server: Open file error\n"); //If it fails, jump to next file
                printf("server: %s\n", nome_arquivo);
                continue;
            }

            //Read file
            char buffer[MAXDATASIZE];
            char output[MAXDATASIZE];
            char *ano = NULL;
            while (fgets(buffer, MAXDATASIZE, file) != NULL) {

                //Append Email info
                if (strstr(buffer, "Email") != NULL) {
                    strcpy(output,buffer);
                }

                //Append Name info
                else if (strstr(buffer, "Nome") != NULL) {
                    strcat(output,buffer);
                }

                //Append Course info
                else if (strstr(buffer, "Formação Acadêmica") != NULL) {
                    strcat(output,buffer);
                }

                //Save current user graduation year data
                else if (strstr(buffer, "Ano de Formatura") != NULL) {
                    ano = strchr(buffer, ':') + 2;
                    ano[strlen(ano)-1] = '\0';
                }

                //Compare the current user graduation year with the request
                if (ano != NULL && strcmp(ano, year) == 0) {
                    //Realloc the message and add the information of the user
                    final = (char *)realloc(final, strlen(final)+strlen(output)+1);
                    strcat(final,output);
                }
            }

            fclose(file);
        }
    }

    //Send final message
    closedir(folder);
    int len;
    len = strlen(final);
    if (send_message(new_fd, final, &len, their_addr, sin_size) == -1) {
        perror("send_message");
        printf("We only sent %d bytes because of the error!\n", len);
    } 
    free(final); //Free memory
    return 1;
}

//===================== Get All users  ================================
int get_all(int new_fd, struct sockaddr_storage their_addr, socklen_t sin_size) {
    DIR *folder;
    struct dirent *entry;

    //Tries to open the users directory
    folder = opendir("../users/.");
    if (folder == NULL) {
        printf("server: open folder error!\n"); //If it fails, return error
        return 0;
    }

    //Allocates message sent dize
    char * final = (char *) malloc(1);
    *final = '\0';  

    //Iterates through all the files
    while ((entry = readdir(folder)) != NULL) {
        if (entry->d_type == DT_REG) {

            //Open the file
            char nome_arquivo[109];
            sprintf(nome_arquivo, "../users/%s", entry->d_name);
            FILE *file = fopen(nome_arquivo, "r");
            if (file == NULL) {
                printf("server: Open file error\n"); //If it fails, jump to next file
                printf("server: %s\n", nome_arquivo);
                continue;
            }

            //Read the data
            char buffer[MAXDATASIZE];
            char output[MAXDATASIZE];
            while (fgets(buffer, MAXDATASIZE, file) != NULL)

                //Append Email info
                if (strstr(buffer, "Email") != NULL) {
                    //Because is the first information use strcpy
                    strcpy(output,buffer);
                } 

                //Append all the other infos
                else { 
                    strcat(output, buffer);
                }

            //Realloc the message and add the information of the user
            final = (char *)realloc(final, strlen(final)+strlen(output)+1);
            strcat(final,output);

            fclose(file);
        }
    }

    //Send the final message
    closedir(folder);
    int len;
    len = strlen(final);
    if (send_message(new_fd, final, &len, their_addr, sin_size) == -1) {
        perror("send_message");
        printf("We only sent %d bytes because of the error!\n", len);
    } 
    free(final); //Free memory
    return 1;
}

//===================== Remove Profile ================================
int remove_profile(char* email) {

    //Check if the file exists
    char nome_arquivo[109];
    sprintf(nome_arquivo, "../users/%s.txt", email);
    FILE *file = fopen(nome_arquivo, "r");
    if (file == NULL){
        printf("server: File not found\n"); //If it fails, return error
        return 0;
    }
    fclose(file);

    //Remove the file
    if (remove(nome_arquivo) == 0) {
        return 1;
    } else {
        printf("server: File remove error\n"); //If it fails, return error
        return 0;
    }
}

//===================== Get picture ======================================
int get_picture(char* email, int new_fd, struct sockaddr_storage their_addr, socklen_t sin_size) {
    char nome_arquivo[109];
    sprintf(nome_arquivo, "../images/%s.png", email); //add the path to the file requested
    FILE *image;
    int numbytes;

    //create image struct
    typedef struct {
        int totalPackets; //Saves total number os packets
        int currentPacket; //Saves current packet
        char imageData[MAXDATASIZE]; //Saves packet data
    } ImagePacket;

    ImagePacket packet;

    //Open the image in binary read mode
    image = fopen(nome_arquivo, "rb");
    if (image == NULL) {
        perror("server: image open error");
        exit(1);
    }

    //Get the image size to calculate the packets
    fseek(image, 0, SEEK_END);
    long imageSize = ftell(image);
    fseek(image, 0, SEEK_SET);
    
    //Calculates the total number of packets needed
    int totalPackets = imageSize/MAXDATASIZE;
    if (imageSize % MAXDATASIZE != 0) {
        totalPackets++;
    }

    //Send each packet, identifying the current packet and the respective data
    int currentPacket = 1;
    while ((numbytes = fread(packet.imageData, 1, MAXDATASIZE, image)) > 0) {
        packet.totalPackets = totalPackets;
        packet.currentPacket = currentPacket;
        printf("Current: %i, Total: %i\n", currentPacket,totalPackets);
        if (sendto(new_fd, &packet, sizeof(ImagePacket), 0, (struct sockaddr *)&their_addr, sin_size) < 0) {
            perror("server: image send error");
            exit(1);
        }
        usleep(100);
        currentPacket++;
    }

    //Close the image
    fclose(image);
    return 1;
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

//===================== Main ================================
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
    hints.ai_socktype = SOCK_DGRAM;
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

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
    
        //Receive the request from the user
        bzero(buf, MAXDATASIZE);
		sin_size = sizeof their_addr;
 		if ((numbytes = recvfrom(sockfd, buf, MAXDATASIZE-1 , 0,(struct sockaddr *)&their_addr, &sin_size)) == -1) {
    		perror("recvfrom");
		    exit(1);
 		}

 		printf("server: got packet from %s\n",inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s));
 		printf("server: packet is %d bytes long\n", numbytes);
 		buf[numbytes] = '\0';
 		printf("server: packet contains \"%s\"\n", buf);

        char first = buf[0];

        //Insert method
        if(first == 'i'){
            memmove(buf, buf+7, strlen(buf)); //remove "insert" from the string

            //Save the data into the database
            if(save_data(buf)){
                printf("server: Client saved in the database\n");
                int len;
                len = strlen("Operation Successful"); //Send operation successful
                if (send_message(sockfd, "Operation Successful", &len, their_addr, sin_size) == -1) {
                    perror("send_message");
                    printf("We only sent %d bytes because of the error!\n", len);
                }
            }

            //If it fails, send operation failed
            else{
                int len;
                len = strlen("Operation Failed");
                if (send_message(sockfd, "Operation Failed", &len, their_addr, sin_size) == -1) {
                    perror("send_message");
                    printf("We only sent %d bytes because of the error!\n", len);
                }
            }
            
	    }

        //Get all method
        else if(first == 'a'){

            //If fails, send operation failed
            if (!get_all(sockfd, their_addr, sin_size)){
                int len;
                len = strlen("Operation Failed");
                if (send_message(sockfd, "Operation Failed", &len, their_addr, sin_size) == -1) {
                    perror("send_message");
                    printf("We only sent %d bytes because of the error!\n", len);
                }
            }
        }

        //Get by email method
        else if(first == 'e'){
            memmove(buf, buf+6, strlen(buf)); //remove "email" from the string

            //If fails, send operation failed
            if(!get_profile(buf, sockfd, their_addr, sin_size)){
                int len;
                len = strlen("Operation Failed");
                if (send_message(sockfd, "Operation Failed", &len, their_addr, sin_size) == -1) {
                    perror("send_message");
                    printf("We only sent %d bytes because of the error!\n", len);
                } 
            }
        }

        //Get by course method
        else if(first == 'c'){
            memmove(buf, buf+7, strlen(buf)); //remove "course" fro string

            //If fails, send operation failed
            if(!get_profile_by_course(buf, sockfd, their_addr, sin_size)){
                int len;
                len = strlen("Operation Failed");
                if (send_message(sockfd, "Operation Failed", &len, their_addr, sin_size) == -1) {
                    perror("send_message");
                    printf("We only sent %d bytes because of the error!\n", len);
                }
            }
        }

        //Get by skill method
        else if(first == 's'){
            memmove(buf, buf+6, strlen(buf)); //remove "skill" from the string

            //If fails, send operation failed
            if(!get_profile_by_skill(buf, sockfd, their_addr, sin_size)){
                int len;
                len = strlen("Operation Failed");
                if (send_message(sockfd, "Operation Failed", &len, their_addr, sin_size) == -1) {
                    perror("send_message");
                    printf("We only sent %d bytes because of the error!\n", len);
                } 
            }
        }

        //Get by year method
        else if(first == 'y'){
            memmove(buf, buf+5, strlen(buf)); //remove "year" from the string

            //If fails, send operation failed
            if(!get_profile_by_year(buf, sockfd, their_addr, sin_size)){
                int len;
                len = strlen("Operation Failed");
                if (send_message(sockfd, "Operation Failed", &len, their_addr, sin_size) == -1) {
                    perror("send_message");
                    printf("We only sent %d bytes because of the error!\n", len);
                }
            }
        }

        //Remove method
        else if(first == 'r'){
            memmove(buf, buf+7, strlen(buf)); //remove "remove" from the string

            //Remove the profile from the database
            if(remove_profile(buf)){
                printf("server: Client removed from the database\n");
                int len;
                len = strlen("Operation Successful"); //Send operation successful
                if (send_message(sockfd, "Operation Successful", &len, their_addr, sin_size) == -1) {
                    perror("send_message");
                    printf("We only sent %d bytes because of the error!\n", len);
                }
            }

            //If fails, send operation failed
            else{
                int len;
                len = strlen("Operation Failed");
                if (send_message(sockfd, "Operation Failed", &len, their_addr, sin_size) == -1) {
                    perror("send_message");
                    printf("We only sent %d bytes because of the error!\n", len);
                } 
            }
        }

        //picture method
        else if(first == 'p'){
            memmove(buf, buf+8, strlen(buf)); //remove "picture" from the string
             if(!get_picture(buf, sockfd, their_addr, sin_size)){
                int len;
                len = strlen("Operation Failed");
                if (send_message(sockfd, "Operation Failed", &len, their_addr, sin_size) == -1) {
                    perror("send_message");
                    printf("We only sent %d bytes because of the error!\n", len);
                }
            }
	    }

        //If the conection closes, print exiting
        else if(strcmp(buf, "") == 0){
            printf("server: Exiting connection from %s\n", s);
            break;
	    }

        //If there is a mistake on the input
        else{
            int len;
            len = strlen("error");
            if (send_message(sockfd, "error", &len, their_addr, sin_size) == -1) {
                perror("send_message");
                printf("We only sent %d bytes because of the error!\n", len);
            }
        }
        bzero(buf, MAXDATASIZE);
    }
    close(sockfd);
    return 0;
}
