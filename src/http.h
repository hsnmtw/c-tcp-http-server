#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <unistd.h> // read(), write(), close()
#include <netdb.h> // For getaddrinfo
#include <pthread.h>
#include <ctype.h>

typedef struct {
    int client_fd;
} http_connrction;

typedef enum {
    OK = 200,
    REDIRECT = 302,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDEN = 403,
    NOT_FOUND = 404,
} http_status;

typedef enum {
    POST,
    PATCH,
    DELETE,
    GET,
    HEAD,
    PUT,
} http_request_method;

typedef struct {
    http_request_method method;
    char* path;
    char* query_string;
} http_route;

void http_start_server();
void *http_handle_connection(void *arg);
int http_extract_route(http_route*route,char*buffer,int size);

#define MAX_BACK_LOCK 80
#define PORT 8080
#define MAX_BUFFER_SIZE 1024
#define MAX_ROUTE_LENGTH 255
#define MAX_QUERY_STRING_LENGTH 255
#define HTTP_GET "GET"   
#define HTTP_POST "POST"  
#define HTTP_PUT "PUT"   
#define HTTP_PATCH "PATCH" 
#define HTTP_HEAD "HEAD"  
#define HTTP_DELETE "DELETE"

int http_extract_route(http_route*route,char*buffer,int size) {
    int i,p,q;
    char method[10] = {0};
    for(i=0;i<size;++i){
        if (buffer[i]==' ') break;
        method[i]=buffer[i];
    }
    for(i++;i<size && p<MAX_ROUTE_LENGTH;++i){
        if (isspace(buffer[i]) || buffer[i]=='?') break;
        route->path[p++]=buffer[i];
    }
    if (buffer[i]=='?') {
        for(i++;i<size && q<MAX_QUERY_STRING_LENGTH;++i){
            if (isspace(buffer[i])) break;
            route->query_string[q++]=buffer[i];
        }
    }
    if (strcmp(method,HTTP_GET   )==0) route->method = GET;
    if (strcmp(method,HTTP_POST  )==0) route->method = POST;
    if (strcmp(method,HTTP_PUT   )==0) route->method = PUT;
    if (strcmp(method,HTTP_PATCH )==0) route->method = PATCH;
    if (strcmp(method,HTTP_HEAD  )==0) route->method = HEAD;
    if (strcmp(method,HTTP_DELETE)==0) route->method = DELETE;
    return p;
}


// [Mon 23 Feb 2026 10:17:34 PM +03]
void *http_handle_connection(void *arg) {
    if (arg == NULL) return NULL;
    char buffer[MAX_BUFFER_SIZE] = {0};
    int r,w,q;
    http_connrction connection = *(http_connrction*)arg;
    free(arg);
    r = read(connection.client_fd,&buffer,MAX_BUFFER_SIZE);
    if (r<1) {
        printf("WRN [http/connection/handle] failed to read from client ... \n");
        return NULL;
    }
    shutdown(connection.client_fd,SHUT_RD);
    http_route route = {0};
    route.path = (char*)malloc(sizeof(char)*MAX_ROUTE_LENGTH);
    route.query_string = (char*)malloc(sizeof(char)*MAX_QUERY_STRING_LENGTH);
    bzero(route.path,MAX_ROUTE_LENGTH);
    bzero(route.query_string,MAX_QUERY_STRING_LENGTH);
    r = http_extract_route(&route,buffer,r);
    printf("[http/connection/handle] [method: %d] [path: %s] [query string: %s]\n", route.method, route.path, route.query_string);
    w = write(connection.client_fd, route.path, r);
    q = strlen(route.query_string);
    if (q > 0) {
        w = write(connection.client_fd, "\r\n\r\n", 4);
        w = write(connection.client_fd, route.query_string, q);
    }
    free(route.path);
    free(route.query_string);
    if (w<1) {
        printf("WRN [http/connection/handle] failed to write to client ... \n");
        return NULL;
    }
    r = read(connection.client_fd, buffer, 128); // some browser may send ACK, just consume it
    shutdown(connection.client_fd, SHUT_RDWR);
    close(connection.client_fd);
    return NULL;
}

// [Mon 23 Feb 2026 09:27:15 PM +03] @hsnmtw
void http_start_server() {
    const int on = 1;
    int sockfd, connfd, len, _; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0) { 
        perror("server socket creation failed...\n"); 
        return; 
    } 
    
    printf("[http/server] Server socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr)); 

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        // Handle error (e.g., print an error message and exit)
        perror("setsockopt(SO_REUSEADDR) failed\n");
    }
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT);
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) { 
        perror("Server socket bind failed...\n"); 
        return; 
    } 
    
    printf("[http/server] Server socket successfully binded..\n"); 

    // Now server is ready to listen and verification 
    if ((listen(sockfd, MAX_BACK_LOCK)) != 0) { 
        perror("Listen failed...\n"); 
        return; 
    } 
    
    printf("[http/server] Server listening..\n"); 
    len = sizeof(cli); 
    
    while (1) {
        // Accept the data packet from client and verification 
        connfd = accept(sockfd, (struct sockaddr*)&cli, &len); 
        if (connfd < 0) { 
            printf("WRN [http/server] server accept failed...\n"); 
            continue; 
        } 
        printf("[http/server] server accept the client...\n"); 

        http_connrction *connection = (http_connrction*)malloc(sizeof(http_connrction));
        connection->client_fd = connfd;
        pthread_t thread;
        pthread_create(&thread,NULL,http_handle_connection,(void*)connection);
        pthread_detach(thread);
    }
    
    shutdown(sockfd, SHUT_RDWR);
    // After chatting close the socket 
    close(sockfd);
}