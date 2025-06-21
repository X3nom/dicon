#define _GNU_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <global_logging.h>
#include <dicon_sockets.h>

int GLOBAL_LOG_LEVEL = 0;

#ifndef COMPUTE_NODE_PATH
#define COMPUTE_NODE_PATH "../../node/bin/main"
#endif


char *create_compute_node_cmd(int port){
    char *cmd;
    asprintf(&cmd, "%s -p %d", 
        COMPUTE_NODE_PATH, port);
    return cmd;
}

void run(char *cmd){
    FILE *out = popen(cmd, "r");
    int exit_status = pclose(out);
    printf("COMPUTE NODE EXITED WITH CODE %d\n", exit_status);
}

void run_loop(int port){
    char *cmd = create_compute_node_cmd(port);
    while(1){
        printf("STARTING COMPUTE NODE\n");
        run(cmd);
    }
}


void show_help_and_exit(){
    printf("\
== DICON NODE MANAGER ==\n\
options: \n\
    -h            | show this menu and exit\n\
    -a            | set address of main server\n\
    -p <port>     | set port to connect to\n\
    -V <int>      | set logging level, default 0\n\
\n\
exiting...\n");
    exit(0);
}


int main(int argc, char *argv[]){
    int opt;

    int port = 4224;
    ip_addr_t ip = (ip_addr_t){0};

    while ((opt = getopt(argc, argv, "hp:V:a:")) != -1) {
        switch (opt) {
            case 'h':
                show_help_and_exit();
                break;

            case 'a':
                ip = ipv4_from_str(optarg);
                break;

            case 'p':
                port = atoi(optarg);
                break;

            case 'V': // set logging level
                GLOBAL_LOG_LEVEL = atoi(optarg);
                break;
        }
    }

    // runs infinitely
    run_loop(port);
    
    return 0;
}