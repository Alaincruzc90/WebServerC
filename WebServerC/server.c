#include <stdio.h>
#include <string.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h>
#include <ctype.h>     //isdigit
#include <stdlib.h>    //atoi
#include <semaphore.h>
#include "ip_management.h"
#include "response_manager.h"
#include "file_manager.h"
#include <sys/types.h>
#include <sys/msg.h>

// Server settings.
int port = 8080;
int max_threads = 15;
int thread_count = 0;
int exit_system = 0;
char* const server_name = "Web Server in C";

// Enum that represents the type of message.
enum RequestMethod {GET, HEAD, POST};

// Thread Binary Semaphore - Thread Management.
sem_t *sem;
static const char *semname = "thread";

// Queue, from which we will receive our next messages.
struct messageBuffer {
    long type;
    char info_log[200];
} message;
int key;
int queueID;

// Structure of our socket.
int socket_desc, new_socket, c;
struct sockaddr_in server, client;

// Our server methods.
void *listen_connection(void *sock);
void check_parameters(int numParam, char *params[]);
void print_clients_request(int client_sock, struct file_status stats, char* content_type);
void print_error_request(int client_sock, struct file_status stats);
void sendlog_info(char *request_log, char *host_log, int methodType);
void writelog_info(int methodType);
int check_number(char *string);

int main(int argc , char *argv[]) {
    
    check_parameters(argc, argv);
    key = getuid();
    queueID = msgget(key, 0600 | IPC_CREAT);
    
    sem = sem_open(semname, O_CREAT, 0777, 1);
    if (sem == SEM_FAILED) {
        fprintf(stderr, "%s\n", "ERROR creating semaphore semname1");
        exit(EXIT_FAILURE);
    }
    if(init_ip_management(5)==0) {
        fprintf(stderr, "%s\n", "ERROR creating semaphore semname1");
        exit(EXIT_FAILURE);
    };
    
    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1){
        printf("Could not create socket");
    }
    
    // Set the socket options to allow the connection to be terminated in connected socket.
    int optval = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
    
    // Prepare the sockaddr_in structure.
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );
    
    // Bind.
    if( bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){
        puts("bind failed");
        return 1;
    }
    puts("bind done");
    
    // Listen.
    listen(socket_desc, 3);
    
    pid_t child_pid = fork();
    if(child_pid == 0){
        writelog_info(1);
    } else if(child_pid<0) {
        printf("Fork failed.");
        exit(-1);
    }
    
    // Accept an incoming connection.
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    
    while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) &&  exit_system == 0){
        
        puts("Connection accepted");
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr.s_addr, ip, sizeof(ip));
        printf("Client's ip -> %s\n",ip);
        
        if(check_ip(client.sin_addr.s_addr) == 1) {
            
            char * message = "Your ip has been suspended for 5 seconds.\n";
            write(new_socket , message , strlen(message));
            close(new_socket);
            
        } else {
            
            // Check if we have reached the maximun amount of usable threads.
            sem_wait(sem);
            
            if (thread_count < max_threads) {
                thread_count++;
                sem_post(sem);
                pthread_t tid;
                pthread_attr_t attr;
                pthread_attr_init(&attr);
                pthread_create(&tid, &attr, listen_connection, (void*)&new_socket);
                
            } else {
                
                // Not enough resources to accept the new connection, so we close it.
                sem_post(sem);
                char * message = "Not enough resources to handle your request. Please, try later.\n";
                write(new_socket, message, strlen(message));
                exclude_ip(client.sin_addr.s_addr);
                close(new_socket);
            }
        }
    }
    
    if (new_socket < 0) {
        perror("accept failed");
        return 1;
    }
    
    // Terminate our connection.
    shutdown(socket_desc, 2);
    close(socket_desc);
    
    destroy_ip_management();
    sem_unlink(semname);
    return 0;
}

// We listen to the client's request and server him the response. It could also fail.
void *listen_connection(void *sock) {
    
    //Get the socket descriptor
    int new_sock = *(int*)sock;
    int read_size;
    char client_message[2000];
    char const error_request[120] = "REQUEST ERROR:\nThe request doesn't complies with the requirements. Please, try again. We only accept GET requests.\n";
    char const exiting_system[120] = "Server is shutting down.\n\0";
    read_size = (int)recv(new_sock , client_message , 2000 , 0);
    
    // Check if the request complies with our requirements.
    int response = check_response(client_message);
    
    //Should the request not fulfill our requirements, we close the connection.
    if (response == 0) {
        write(new_sock, error_request, strlen(error_request));
        close(new_sock);
        sem_wait(sem);
        thread_count--;
        sem_post(sem);
        pthread_exit(NULL);
    }
    
    char *message = "\n";
    write(new_sock , message , strlen(message));
    
    // If the response complies with the requirements, then execute what the user wants.
    if(response == 1) {
        
        char* request = get_request(client_message);
        char* host = get_host(client_message);
        char* type = get_type(request);
        
        if( strcmp(request, "/exit") == 0) {
            write(new_sock, exiting_system, strlen(exiting_system));
            exit_system = 1;
            close(new_sock);
            sem_wait(sem);
            thread_count--;
            sem_post(sem);
            pthread_exit(NULL);
        }
        
        // Send the request to print.
        sendlog_info(request, host, 1);
        
        // Check if we got a "/" request, which will be modified into a index.html.
        char *final_request = malloc(256);
        if(strlen(request) <= 1) {
            strcpy(final_request, "/index.html");
        } else {
            strcpy(final_request, request);
        }
        
        // Check that we didn't get a request like host:7070.
        // If we did, we should get change the request for index.html.
        if (strlen(final_request) > 1) {
            
            struct file_status file_stats = get_status(final_request, host, type);
            if(file_stats.code == 200 || file_stats.code == 204){
                print_clients_request(new_sock, file_stats, type);
            } else {
                print_error_request(new_sock, file_stats);
            }
            
            // We free variables that were created using malloc.
            free(request);
            free(host);
            if (file_stats.size > 0 && strcmp(file_stats.file_bytes,"") != 0) {
                free(file_stats.file_bytes);
            }
        } else {
            
            // Return if the directory either doesn't exists or fails to open.
            // Send an error message if something fails.
            struct file_status stats;
            stats.code = 501;
            stats.size = 0;
            strcpy(stats.code_name, "Server Error");
            stats.file_bytes = "";
            print_error_request(new_sock, stats);
        }
    }
    if(read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1) {
        perror("recv failed");
    }
    
    // When finished, we close the clients socket and change the thread_count.
    close(new_sock);
    sem_wait(sem);
    thread_count--;
    sem_post(sem);
    pthread_exit(NULL);
}

// Check for user inputs regarding maximun thread usage and ports.
// We use -p to set the port and -m to set the thread count.
void check_parameters(int numParam, char *params[]) {
    int counter;
    for (counter = 1; counter<numParam-1; counter++) {
        if(strcmp(params[counter],"-p") == 0 || strcmp(params[counter],"-port") == 0) {
            if (counter+1>=numParam) {
                break;
            }
            char *string = params[counter+1];
            if (check_number(string) == 1) {
                int portNumber = atoi(string);
                if (portNumber>1024) {
                    port = portNumber;
                    printf("Using port -> %d.\n", portNumber);
                } else {
                    printf("Can't use ports below 1024. Using 7070 for now.\n");
                }
            } else {
                printf("%s is not a valid number. Using port 7070 for now.\n", string);
            }
        }
        if(strcmp(params[counter],"-m") == 0) {
            if (counter+1>=numParam) {
                break;
            }
            char *string = params[counter+1];
            if (check_number(string) == 1) {
                int threads = atoi(string);
                if (threads>=3) {
                    max_threads = threads;
                    printf("Using a maximun of %d threads.\n",threads);
                } else {
                    printf("Can't use less than 3 threads. Using 15 threads for now.\n");
                }
            } else {
                printf("%s is not a valid number. Using 15 threads for now.\n", string);
            }
        }
    }
}

// Method to check if a string can be cast as a number.
int check_number(char *string){
    int i = 0;
    while (i<sizeof(string)) {
        if (!isdigit(*string+1)) {
            return 0;
        }
        i++;
    }
    return 1;
}

// If the client's request resulted in succes, we print a message with the requested file.
void print_clients_request(int client_sock, struct file_status stats, char* content_type) {
    
    // We need to allocate enough space for the message.
    // It is important for it to be at least as big as the file we are going to send, plus the header size.
    // It need to be dinamically allocated, because C won't allow it otherwise.
    int const size_request = stats.size+1024;
    char* client_response = malloc(size_request*sizeof(char*));
    char code[4];
    char length_bytes[40];
    char text_date[100];
    char text_modified[100];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(text_date, sizeof(text_date)-1, "%a, %d %b %Y %H:%M:%S %Z", t);
    strftime(text_modified, sizeof(text_modified)-1, "%a, %d %b %Y %H:%M:%S %Z", localtime((time_t*)&(stats.attrib.st_ctimespec)));
    snprintf(code, sizeof(code), "%d", stats.code);
    snprintf(length_bytes, sizeof(length_bytes), "%d", stats.size);
    
    // The message needs to be concatenated.
    strcat(client_response, "HTTP/1.1 ");
    strcat(client_response, code);
    strcat(client_response, " ");
    strcat(client_response, stats.code_name);
    strcat(client_response, "\n");
    strcat(client_response, "Date: ");
    strcat(client_response, text_date);
    strcat(client_response, "\n");
    strcat(client_response, "Server: ");
    strcat(client_response, server_name);
    strcat(client_response, "\n");
    strcat(client_response, "Host: ");
    strcat(client_response, server_name);
    strcat(client_response, "\n");
    strcat(client_response, "Last-Modified: ");
    strcat(client_response, text_modified);
    strcat(client_response, "\n");
    strcat(client_response, "Content-Length: ");
    strcat(client_response, length_bytes);
    strcat(client_response, "\n");
    strcat(client_response, "Content-Type: ");
    
    // Check the type of content we are receiving, and depeding on it
    // send it's corresponding Content-Type.
    if(strcmp(content_type, "html") == 0) {
        strcat(client_response, "text/html;charset=UTF-8\n");
    } else if(strcmp(content_type, "jpg") == 0) {
        strcat(client_response, "image/jpeg\n");
    } else if(strcmp(content_type, "css")  == 0) {
        strcat(client_response, "text/css\n");
    } else {
        strcat(client_response, "text/plain\n");
    }
    strcat (client_response, "Connection: keep-alive\r\n\r\n");
    
    // We send the header and the file to the client.
    send(client_sock, client_response, strlen(client_response), 0);
    send(client_sock, stats.file_bytes, stats.size, 0);
    send(client_sock, "\n", 1, 0);
    
}

//Error message printed to client, if there was a mistake with his request.
void print_error_request(int client_sock, struct file_status stats) {
    
    //We initialized our array with 0's.
    //This array will contain the error message, that will be sent to the client.
    char client_response[1024] = {"\0"};
    char code[4];
    char text_date[100];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(text_date, sizeof(text_date)-1, "%a, %d %b %Y %H:%M:%S %Z", t);
    snprintf(code, sizeof(code), "%d", stats.code);
    strcat(client_response, "HTTP/1.1 ");
    strcat(client_response, code);
    strcat(client_response, " ");
    strcat(client_response, stats.code_name);
    strcat(client_response, "\n");
    strcat(client_response, "Date: ");
    strcat(client_response, text_date);
    strcat(client_response, "\n");
    strcat(client_response, "Server: ");
    strcat(client_response, server_name);
    write(client_sock, client_response, strlen(client_response));
    send(client_sock,"\n",1,0);
}

// Send message to the queue, with log info to write.
void sendlog_info(char *request_log, char *host_log, int methodType) {
    // Size of the message that will be send.
    unsigned long size = strlen(request_log) + strlen(host_log);
    char text_date[100];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(text_date, sizeof(text_date)-1, "%a, %d %b %Y %H:%M:%S %Z", t);
    char *log_info = (char *)malloc(size+150);
    strcat(log_info, "Date: ");
    strcat(log_info, text_date);
    strcat(log_info, " | Request: ");
    strcat(log_info, request_log);
    strcat(log_info, " | Host: ");
    strcat(log_info, host_log);
    strcat(log_info, "\0");
    message.type = 1;
    strncpy(message.info_log, log_info, sizeof(message.info_log));
    int result = msgsnd(queueID, &message, sizeof(message), 0);
    if(result<0){
        perror("Error sending message.");
    }
    free(log_info);
}

// Recieve message from queue and write it to logs.
void writelog_info(int methodType) {
    ssize_t result;
    while((result = msgrcv(queueID, &message, sizeof(message), 1, 0))){
        if(result<0) {
            perror("Error recieving message.");
        } else {
            write_log(message.info_log, methodType);
            printf("%s\n",message.info_log);
        }
    }
}
