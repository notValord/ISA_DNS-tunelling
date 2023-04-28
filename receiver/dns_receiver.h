//
// Created by vmvev on 11/1/2022.
//

#ifndef PROJECT_DNS_RECEIVER_H
#define PROJECT_DNS_RECEIVER_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>

#define INPUT_ERR -1
#define FAILURE -2
#define SHUT_DOWN -3
#define DNS_PORT 53
#define DNS_SIZE 1024
#define NAME_SIZE 255

#define SEND_TRANC_FLAG 0x0200
#define REC_NORM_FLAG 0x8000
#define REC_ERR_FLAG 0x8001

#define RET_SKIP -1
#define RET_ERR 2
#define RET_TRUNC 1
#define RET_NORM 0

typedef struct Rec_arguments{
        char base_host[NAME_SIZE];
        char dir_path[NAME_SIZE];

}Rec_args;

/// Prints help
void print_help();

/// Initiates the receiver arguments with passed values
int parse_args(int argc, char* argv[], Rec_args* args);

/**
 * Gets the decoded filepath where the data will be stored from dns question
 * @return Number of chars read from the packet
 */
int get_filepath(char* packet, char* filepath);

/**
 * Checks whether the server base host and the one dns query are the same
 * @param read Number of chars read from the packet
 */
bool check_domain(char* packet_start, char* domain, int* read);

/// Creates directories in dirpath
void create_recursive_dir(char* dirpath);

/**
 * Create filepath and save data into the file
 * @param append Flag whether the data should be appended or written
 */
void save_data(char* dirpath, char* filepath, uint8_t* data, int data_len, bool append);

/**
 * Parses the packet and saves the data into 'filepath'
 * @param length Is returned with the length of the question name
 * @return Flag representing how the parsing has ended
 */
int parse_packet(char* packet, char* filepath, Rec_args* args, int* length, struct in_addr *src, int* file_len);

/// Prepares the passed udp socket for the connection and data transmit
void prepare_udp_sock(int* udp_socket, struct sockaddr_in* rec_udp_addr);

/// Prepares the passed sockets for the tcp connection and data transmit on accept_tcp_socket
int prepare_tcp_sock(int* tcp_socket, int* accept_tcp_sock, struct sockaddr_in* rec_tcp_addr, struct sockaddr_in* send_tcp_addr);

#endif //PROJECT_DNS_RECEIVER_H
