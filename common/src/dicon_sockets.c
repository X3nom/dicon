#include <dicon_sockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>



int tcp_connect(const char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    // Create client socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Socket creation failed");
        return -1;
    }
    // printf("Client socket created successfully\n");

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        return -1;
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}


int dic_conn_disconnect(dic_connection_t *conn){
    // Close the socket
    close(conn->sockfd);
    return 0;
}


// DIC CONNECTION IMPLEMENTATION ===============================================================

/** returns malloced dic_connection_t
 * - socket may be -1 (connection failed)
 * - recommended to check for `conn->sockfd != -1` or `dic_conn_tryconnect(conn) == 0` */
dic_connection_t *dic_conn_new(char *ip, int port){
    int sockfd = tcp_connect(ip, port);
    dic_connection_t *conn = (dic_connection_t*)malloc(sizeof(dic_connection_t));
    
    conn->sockfd = sockfd;
    strcpy(conn->ip, ip);
    conn->port = port;

    pthread_mutex_init(&conn->send_mut, NULL);
    pthread_mutex_init(&conn->recv_mut, NULL);

    return conn;
}

void dic_conn_destroy(dic_connection_t *conn){
    dic_conn_disconnect(conn);
    free(conn);
}

int dic_conn_send(dic_connection_t *conn, const char *message){
    int sockfd = conn->sockfd;
    // Send data to the server
    send(sockfd, message, strlen(message), 0);
}

int dic_conn_recv(dic_connection_t *conn, char *buffer, int buffer_size){
    int sockfd = conn->sockfd;
    // Receive data from the server
    int bytes_received = recv(sockfd, buffer, buffer_size, 0);
    if (bytes_received == -1) {
        perror("Receive failed");
        close(sockfd);
        conn->sockfd = -1; // set sockfd to closed/error state
        return 1;
    }
    buffer[bytes_received] = '\0';  // Null-terminate the received data
    printf("Received from server: %s\n", buffer);
    return 0;
}


int dic_conn_tryconnect(dic_connection_t *conn){
    if(conn->sockfd != -1) return 0;
    int sockfd = tcp_connect(conn->ip, conn->port);
    if(sockfd == -1) return 1;
    conn->sockfd = sockfd;
    return 0;
}




int main(){
    dic_connection_t *conn = dic_conn_new("0.0.0.0", 1234);

    dic_conn_send(conn, "hello world");

    dic_conn_disconnect(conn);

    dic_conn_tryconnect(conn);

    dic_conn_send(conn, "hello again");
    
    dic_conn_destroy(conn);
}