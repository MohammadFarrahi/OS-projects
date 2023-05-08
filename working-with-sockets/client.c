#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>
#include "constants.h"


struct Broadcast {
    int port;
    int udp_fd;
    int room_id;
    int order;
    struct sockaddr_in bc_address;
    char dumy_buff[1024];
};
typedef struct Broadcast Broadcast;

void die_with_error(char const* message) {
    write(1, message, strlen(message));
    exit(EXIT_FAILURE);
}

void handle_delay(int sig) {
    write(1, "Your time for asking/answering question is over.\n", 49);
    write(1, "A default will be considered as yours.\n", 38);
}

int connect_server(int port) {
    int fd;
    struct sockaddr_in server_address;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(port); 
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) { // checking for errors
        die_with_error("Error in connecting to server. Exiting...\n");
    }
    write(1, "Connection to server was successful.\n\n", 38);
    return fd;
}

void setup_udp_connection(Broadcast* bc_info) {
    int sock_fd, broadcast = 1, opt = 1;

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_info->bc_address.sin_family = AF_INET; 
    bc_info->bc_address.sin_port = htons(bc_info->port);
    bc_info->bc_address.sin_addr.s_addr = inet_addr("192.168.1.255");

    bind(sock_fd, (struct sockaddr *)&(bc_info->bc_address), sizeof(bc_info->bc_address));
    bc_info->udp_fd = sock_fd;
    return;
}

void request_for_room(int fd_to_server, char buffer[1024]) {
    int entered_num;
    while(1){
        write(1, "--> Computer science: 1\n", 24);
        write(1, "--> Electricity: 2\n", 19);
        write(1, "--> Mechanical engineering: 3\n", 30);
        write(1, "--> civil engineering: 4\n", 25);
        write(1, "Please enter your subject: ", 27);
        read(0, buffer, 1024);
        entered_num = atoi(buffer);
        if((COMPUTER_T <= entered_num && entered_num <= CIVIL_T)){ break; }
        write(1, "You entered invalid input!\n", 27);
    }
    send(fd_to_server, buffer, strlen(buffer), 0);
    memset(buffer, 0, strlen(buffer));
    
    if(recv(fd_to_server, buffer, 1024, 0) > 0) { write(1, buffer, strlen(buffer)); }
    else { die_with_error("Request failed. Exiting...\n"); }
    memset(buffer, 0, strlen(buffer));
}

Broadcast get_broadcast_info(int fd_to_server, char buffer[]) {
    Broadcast broadcast_info;
    int port, room_id, order;
    recv(fd_to_server, buffer, 1024, 0);
    if((sscanf(buffer, "%d %d %d", &port, &room_id, &order)) == 3
        && port && room_id && order) {
        write(1,"Ready to connect to room(#broadcast_port #room_id #order) : ", 61);
        write(1, buffer, strlen(buffer)); write(1, "\n", 1);
        memset(buffer, 0, strlen(buffer));

        broadcast_info.port = port;
        broadcast_info.room_id = room_id;
        broadcast_info.order = order;
    }
    else {
        write(1, buffer, strlen(buffer));
        die_with_error(" recieved form server. information is invalid. Exiting...\n");
    }
    return broadcast_info;
}

int get_best_answer_from_user(char buffer[]){
    int bytes_read, answer_num;
    while(1){
        write(1,"Enter the number of best answer: ", 34);
        flush_terminal();
        alarm(30);
        bytes_read = read(0, buffer, 1024);
        alarm(0);
        if(bytes_read < 0){
            return 1;
        }
        else if((answer_num = atoi(buffer)) && 1 <= answer_num && answer_num <= MAX_ROOM_CAPACITY-1){
            return answer_num;
        }
        else {
            write(1,"Your answer is not valid! ", 26);
        }
    }
}

void get_answers(Broadcast* bc_info, char buffer[]) {
    int iter;
    char answers [MAX_ROOM_CAPACITY][1024] = {0};
    for(iter = 0; iter < MAX_ROOM_CAPACITY-1; iter++){
        recv(bc_info->udp_fd, answers[iter], 1024, 0);
        int bytes_written = sprintf(buffer + strlen(buffer), "answer %d: %s", iter+1, answers[iter]);
        write(1, buffer+strlen(buffer)-bytes_written, bytes_written);
    }
    iter = get_best_answer_from_user(answers[iter]);
    sprintf(buffer + strlen(buffer), "best answer: %d\n", iter);

    sprintf(answers[MAX_ROOM_CAPACITY-1], "best answer: %s", answers[iter-1]);
    sendto(bc_info->udp_fd, answers[MAX_ROOM_CAPACITY-1], strlen(answers[MAX_ROOM_CAPACITY-1]), 0,(struct sockaddr *)&(bc_info->bc_address), sizeof(bc_info->bc_address));
    recv(bc_info->udp_fd, bc_info->dumy_buff, 1024, 0);
}

void process_questioning(Broadcast* bc_info, int fd_to_server, char buffer[]) {
    char Q_buff[1024]; int bytes_read;
    write(1, "It is your turn to ask a question. Enter your question: (you have limited time)\n", 82);
    flush_terminal();
    alarm(90);
    bytes_read = read(0, Q_buff, 1024);
    alarm(0);
    if(bytes_read < 1) { strcpy(Q_buff, "pass\n"); }
    sprintf(buffer, "Q&A %d: #room_id / %d: #client_order\n%s", bc_info->room_id, bc_info->order, Q_buff);
    sendto(bc_info->udp_fd, Q_buff, strlen(Q_buff), 0,(struct sockaddr *)&(bc_info->bc_address), sizeof(bc_info->bc_address));
    recv(bc_info->udp_fd, bc_info->dumy_buff, 1024, 0);
    
    get_answers(bc_info, buffer);
    send(fd_to_server, buffer, strlen(buffer), 0);
    memset(buffer, 0, 2048);
}

void send_client_answer(Broadcast* bc_info, char buffer[]){
    write(1, "Your turn! Answer the question: (you have limited time)\n", 56);
    flush_terminal();
    alarm(60);
    int answer_len = read(0, buffer, 1024);
    alarm(0);
    if(answer_len <= 1){
        answer_len = sprintf(buffer, "pass\n");
    }
    sendto(bc_info->udp_fd, buffer, answer_len, 0,(struct sockaddr *)&(bc_info->bc_address), sizeof(bc_info->bc_address));
    recv(bc_info->udp_fd, bc_info->dumy_buff, 1024, 0);
}

void process_answering(Broadcast* bc_info, int fd_to_server, int Q_owner, char buffer[]){
    recv(bc_info->udp_fd, buffer, 1024, 0);
    write(1, "Question to answer:\n", 20);
    write(1, buffer, strlen(buffer));

    for(int iter = 1; iter <= MAX_ROOM_CAPACITY; iter++){
        memset(buffer, 0, 1024);
        if((Q_owner > bc_info->order && iter == bc_info->order) ||
            (Q_owner < bc_info->order && iter == bc_info->order-1)){
            send_client_answer(bc_info, buffer);
            continue;
        }
        else {
            int bytes_read = sprintf(buffer, "answer %d: ", iter);
            recv(bc_info->udp_fd, buffer + bytes_read, 1024 - bytes_read, 0);
            if(iter == MAX_ROOM_CAPACITY){
                write(1, buffer + bytes_read, strlen(buffer) - bytes_read);
            }
            else { write(1, buffer, strlen(buffer)); }
        }
    }
    memset(buffer, 0, 1024);
}

void flush_terminal(){
    char dummy_buff[1024];
    fd_set master_set;
    struct timeval tv; 
    int max_sd = 0, status, is_any = FALSE;

    FD_ZERO(&master_set);
    FD_SET(0, &master_set);
    tv.tv_sec = 0; tv.tv_usec = 1000;

    while(1){
        status = select(max_sd+1, &master_set, NULL, NULL, &tv);
        if(status == 0){
            if(is_any){ write(1, "*****Your previous unasked writings to terminal, has been ignored******\n", 80); }
            return;
        }
        else { is_any = TRUE; read(0, dummy_buff, 1024); }
    }
}

int main(int argc, char const *argv[]) {
    int tcp_fd, server_port;
    char buffer[2048] = {0};
    Broadcast broadcast_info;
    struct sockaddr_in bc_address;

    if(argc < 2 || !(server_port = atoi(argv[1]))){
        die_with_error("Port is not specified or appropriate. Exiting...\n");
    }
    tcp_fd = connect_server(server_port);

    request_for_room(tcp_fd, buffer);
    broadcast_info = get_broadcast_info(tcp_fd, buffer);
    setup_udp_connection(&broadcast_info);
    
    signal(SIGALRM, handle_delay);
    siginterrupt(SIGALRM, 1);
    for(int iter = 1; iter <= MAX_ROOM_CAPACITY; iter++){
        printf("\nRound %d:\n", iter);
        if(iter == broadcast_info.order){
            process_questioning(&broadcast_info, tcp_fd, buffer);
            continue;
        }
        process_answering(&broadcast_info, tcp_fd, iter, buffer);
    }
    flush_terminal();
    close(tcp_fd);
    return 0;
}