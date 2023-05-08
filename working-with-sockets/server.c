#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "constants.h"


struct Pending {
    int id;
    int clients_count;
    int clients_fd[MAX_ROOM_CAPACITY];
};
typedef struct Pending Pending;

void die_with_error(char const* message) {
    write(1, message, strlen(message));
    exit(EXIT_FAILURE);
}

int add_client_to_room(int client_fd, int subject_type, Pending pending_rooms[]){
    char result_message[100] = "Request was successful. Please wait for others to join.\n";
    int is_room_full = FALSE;
    Pending* pr = NULL;
    
    if(COMPUTER_T <= subject_type && subject_type <= CIVIL_T) {
        pr = &pending_rooms[subject_type-1];
        pr->id = pr->id == 0 ? subject_type : pr->id;

        if(pr->clients_count >= MAX_ROOM_CAPACITY) {
            pr->id += 5;
            pr->clients_count = 0;
        }
        pr->clients_fd[pr->clients_count++] = client_fd;
        send(client_fd, result_message, strlen(result_message), 0);
        if(pr->clients_count == MAX_ROOM_CAPACITY){  is_room_full = TRUE; }
    }
    else {
        send(client_fd, "Subject type is invalid.\n", 25, 0);
        is_room_full = ERROR;
    }
    return is_room_full;
}

void send_broadcast_info(Pending* room, int port){
    char buffer[20]; int str_size;
    for(int i = 0; i < room->clients_count; i++){
        str_size = sprintf(buffer, "%d %d %d", port, room->id, i+1);
        send(room->clients_fd[i], buffer, str_size, 0);
        memset(buffer, 0, str_size);
    }
}

int save_client_content(int client_fd, char* buffer, int content_size) {
    int room_id;
    if(sscanf(buffer, "Q&A %d", &room_id) == 0 || room_id == 0){ return ERROR; }

    int file_fd = open(SUBJECT_FILE_NAMES[room_id % 5], O_CREAT | O_APPEND | O_RDWR, S_IRWXU);
    write(file_fd, buffer, content_size);
    close(file_fd);
    return room_id;
}

int setup_server(int port) {
    struct sockaddr_in address;
    int server_fd; int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(port);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);
    return server_fd;
}

int accept_client(int server_fd) {
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);
    return client_fd;
}

int process_room(Pending* room, int room_status, int subject_type, int client_fd, int room_port){
    switch(room_status) {
        case ERROR:
            printf("Adding Client fd=%d failed.\n", client_fd);
            printf("Client sent invalid subject type = %d.\n\n", subject_type);
            break;
        case TRUE:
            printf("Cilent fd=%d with order=%d added.\n", client_fd, room->clients_count);
            printf("Capacity of room id=%d : %d.\n", room->id, room->clients_count);
            printf("Sending port = %d to clients of room...\n\n", ++room_port);
            send_broadcast_info(room, room_port);
            break;
        case FALSE:
            printf("Cilent fd=%d with order=%d added.\n", client_fd, room->clients_count);
            printf("Capacity of room id=%d : %d.\n",room->id, room->clients_count);
            break;
    }
    return room_port;
}

void remove_from_pending_room(int client_fd, Pending pending_rooms[]){
    int found = FALSE;
    for(int iter = COMPUTER_T; iter <= CIVIL_T; iter++){
        if(pending_rooms[iter-1].clients_count >= MAX_ROOM_CAPACITY) { continue; }
        for(int i = 0; i < pending_rooms[iter-1].clients_count; i++){
            if(!found && client_fd == pending_rooms[iter-1].clients_fd[i]){  found = TRUE; }
            if(found){ pending_rooms[iter-1].clients_fd[i] = pending_rooms[iter-1].clients_fd[i+1]; }
        }
        if(found){ pending_rooms[iter-1].clients_count--; return; }
    }
}

int main(int argc, char const *argv[]) {
    int server_fd, new_client_fd, max_sd, server_port, last_room_port = 49152;
    fd_set master_set, working_set;
    Pending pending_rooms[4];
    char in_buffer[1024] = {0};

    if(argc < 2 || !(server_port = atoi(argv[1]))){
        die_with_error("Port is not specified or appropriate. Exiting...\n");
    }
    server_fd = setup_server(server_port);
    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);

    write(1, "Server is running\n", 18);

    for(int i = COMPUTER_T; i <= CIVIL_T; i++){ unlink(SUBJECT_FILE_NAMES[i]); }
    memset(pending_rooms, 0, sizeof(pending_rooms));
    while (1) {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);

        for (int fd = 0; fd <= max_sd; fd++) {
            if (FD_ISSET(fd, &working_set)) {
                
                if (fd == server_fd) { // new client
                    new_client_fd = accept_client(server_fd);
                    FD_SET(new_client_fd, &master_set);
                    if (new_client_fd > max_sd){ max_sd = new_client_fd; }
                    printf("New client connected. fd = %d\n", new_client_fd);
                }
                else { // client sending msg
                    int bytes_received, subject_type;
                    bytes_received = recv(fd , in_buffer, 1024, 0);
                
                    if (bytes_received == 0) { // EOF
                        printf("client fd = %d closed\n", fd);
                        remove_from_pending_room(fd, pending_rooms);
                        close(fd);
                        FD_CLR(fd, &master_set);
                        continue;
                    }
                    if((subject_type = atoi(in_buffer))) { // room request
                        int room_status = add_client_to_room(fd, subject_type, pending_rooms);

                        if(last_room_port == server_port){ last_room_port++; }
                        last_room_port = process_room(&pending_rooms[subject_type-1], room_status, subject_type, fd, last_room_port);
                    }
                    else { //sending Q&A to save
                        if(save_client_content(fd, in_buffer, bytes_received) == ERROR){
                            printf("Un acceptable message from client %d: %s\n\n", fd, in_buffer);
                        }
                    }
                    memset(in_buffer, 0, 1024);
                }
            }
        }
    }
    return 0;
}