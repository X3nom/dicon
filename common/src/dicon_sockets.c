#include <dicon_sockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// locks mutex, executes code inside the "block", then unlocks
#define MUTEX_BLOCK(mutex, code...) \
    pthread_mutex_lock(mutex); \
    code \
    pthread_mutex_unlock(mutex)



ip_addr_t ip_from_str(int family, const char *ip_str){
    ip_addr_t ip;
    ip.family = family;
    inet_pton(family, ip_str, &ip.addr);
    return ip;
}
ip_addr_t ipv4_from_str(const char *ip_str){
    return ip_from_str(AF_INET, ip_str);
}
ip_addr_t ipv6_from_str(const char *ip_str){
    return ip_from_str(AF_INET6, ip_str);
}



// takes ip in binary representation
int tcp_connect(ip_addr_t ip, int port) {
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
    if (ip.addr.ipv4 <= 0) {
        perror("Invalid address");
        close(sockfd);
        return -1;
    }
    server_addr.sin_addr.s_addr = ip.addr.ipv4;

    // Connect to the server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}


int dic_conn_disconnect(dic_conn_t *conn){
    // Close the socket
    close(conn->sockfd);
    conn->sockfd = -1;
    return 0;
}


// DIC CONNECTION IMPLEMENTATION ===============================================================

/** returns malloced dic_connection_t
 * - socket may be -1 (connection failed)
 * - recommended to check for `conn->sockfd != -1` or `dic_conn_tryconnect(conn) == 0` */
dic_conn_t *dic_conn_new(ip_addr_t ip, int port){
    int sockfd = tcp_connect(ip, port);
    dic_conn_t *conn = (dic_conn_t*)malloc(sizeof(dic_conn_t));
    
    conn->sockfd = sockfd;
    conn->ip = ip;
    conn->port = port;

    pthread_mutex_init(&conn->send_mut, NULL);
    pthread_mutex_init(&conn->recv_mut, NULL);

    return conn;
}

void dic_conn_destroy(dic_conn_t *conn){
    dic_conn_disconnect(conn);
    free(conn);
}

#define SEND_STR -1
// Not thread-safe on its own
int dic_conn_send(dic_conn_t *conn, const char *message, int len){
    if(len == SEND_STR) len = strlen(message);
    int sockfd = conn->sockfd;
    // Send data to the server
    // MUTEX_BLOCK(&conn->send_mut,
    send(sockfd, message, len, 0);
    // );

    return 0;
}

// Not thread-safe on its own
int dic_conn_recv(dic_conn_t *conn, char *buffer, int buffer_size, bool null_terminate){
    int sockfd = conn->sockfd;
    // Receive data from the server
    // MUTEX_BLOCK(&conn->recv_mut,
    int bytes_received = recv(sockfd, buffer, buffer_size, 0);
    // );
    if (bytes_received == -1) {
        perror("Receive failed");
        close(sockfd);
        conn->sockfd = -1; // set sockfd to closed/error state
        return 1;
    }
    if(null_terminate) buffer[bytes_received] = '\0';  // Null-terminate the received data
    // printf("Received from server: %s\n", buffer);
    return 0;
}


int dic_conn_tryconnect(dic_conn_t *conn){
    if(conn->sockfd != -1) return 0;
    int sockfd = tcp_connect(conn->ip, conn->port);
    if(sockfd == -1) return 1;
    conn->sockfd = sockfd;
    return 0;
}



/* SERVER STUFF --------*/

dic_listen_conn_t *dic_conn_new_listening(int port){
    int server_fd;
    struct sockaddr_in server_addr;
    // socklen_t addr_size = sizeof(server_addr);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    server_addr.sin_port = port;

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_fd, 8) == -1) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    dic_conn_t *conn = (dic_conn_t*)malloc(sizeof(dic_conn_t));
    conn->port = port;
    conn->sockfd = server_fd;
    conn->ip = NONE_IP_ADDR;

    pthread_mutex_init(&conn->recv_mut, NULL);
    pthread_mutex_init(&conn->send_mut, NULL);

    return conn;
}


dic_conn_t *dic_conn_accept(dic_listen_conn_t *listen_conn){
    
    struct sockaddr_in client_addr;
    socklen_t addr_size = sizeof(client_addr);

    // Accept client connection
    int client_fd = accept(listen_conn->sockfd, (struct sockaddr*)&client_addr, &addr_size);
    if (client_fd == -1) {
        perror("Accept failed");
        // exit(EXIT_FAILURE);
        return NULL;
    }

    dic_conn_t *conn = (dic_conn_t*)malloc(sizeof(dic_conn_t));

    conn->sockfd = client_fd;
    conn->port = client_addr.sin_port;
    conn->ip.addr.ipv4 = client_addr.sin_addr.s_addr;

    pthread_mutex_init(&conn->recv_mut, NULL);
    pthread_mutex_init(&conn->send_mut, NULL);

    return conn;   
}