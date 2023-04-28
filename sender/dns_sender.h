/**
* Project name: Tunelování datových přenosů přes DNS dotazy
* Author: Veronika Molnárová
* Date: 6.11 2022
* Subject: Síťové aplikace a správa sítí
*/


#ifndef PROJECT_DNS_SENDER_H
#define PROJECT_DNS_SENDER_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define STDIN "STDIN"
#define CONF_FILE "/etc/resolv.conf"        /// File with default DNS server

#define DNS_SIZE 1024           /// DNS max packet length, 512 should be enough
#define MAX_DOMAIN_LEN 255      /// Maximal length of question in dns query
#define DNS_PORT 53             /// Port number for DNS
#define IP_SIZE 16              /// Length of IPv4 IP address
#define MAX_CHARS_FOR_CHUNK 39  /// Max number of characters to be read before coding per label
#define MAX_CHUNK 63            /// Max number of coded characters per label

#define INPUT_ERR -1          /// Arguments error
#define FAILURE -2              /// Other failures
#define FILE_ERR -3             /// Error while working with file

#define SEND_TRUNC_FLAG 0x0200
#define SEND_FLAG 0x0000
#define SEND_CLASS 0x0001
#define SEND_TYPE 0x0001

typedef struct Sender_args{
        char dns_ip[IP_SIZE];               /// IP of the server to connect to
        char base_host[MAX_DOMAIN_LEN];
        char file_path[MAX_DOMAIN_LEN];     /// Filepath where the data will be stored on the server
        char input[MAX_DOMAIN_LEN];         /// Either stdin or file to be read
}Send_args;

/// Prints help
void print_help();

/// Initiates the sender arguments with passed values
int parse_args(int argc, char* argv[], Send_args* args);

/// Prints the sender arguments
void debug_args(Send_args* args);

/// Prepare the base domain to dns format to be sent (replace '.' with length of next label in hex)
/// google.com ==> change to '0x06'google'0x03'com
void prepare_domain(char* encoded_data, const char* domain);

/// Encodes the filepath with its end marked with '-' into dns format
/// Is expected to fit into one dns query without parsing
void prepare_file_path(char* encoded_path, const char* path);

/**
 *  Reads data from the input, splits into labels and codes
 *  @param max_read Maximal length of coded data which fits into dns question
 *  @return If end of file wasn't reached returns the 1 as a truncation flag, else returns 0
 */
int read_file(char* encoded_data, FILE* file, int max_read, bool std, int* chunk_size, char* filepath, unsigned short id);

/**
 *  Creates a DNS packet in 'buffer' and fills it with passed data
 * @param encoded_path Coded filepath where the data will be stored
 * @param encoded_data Coded data
 * @param encoded_base Base domain in DNS format
 * @param trunct Flag whether the packet should be truncated
 * @return The length of the created question name
 */
int build_packet(char* buffer, char* encoded_path, char* encoded_data, char* encoded_base, int trunct, unsigned short id);

/// Creates and initiates a udp socket
void prepare_udp_sock(int* udp_socket, struct sockaddr_in* server_addr, Send_args* args);

/// Creates, initiate and connect a tcp socket to the server
void prepare_tcp_sock(int* tcp_socket, struct sockaddr_in* server_tcp_addr, Send_args* args);

#endif //PROJECT_DNS_SENDER_H
