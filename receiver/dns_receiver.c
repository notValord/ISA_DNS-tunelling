/**
* Project name: Tunelování datových přenosů přes DNS dotazy
* Author: Veronika Molnárová
* Date: 11.11 2022
* Subject: Síťové aplikace a správa sítí
*/

#define MSG_CONFIR 2048     // const for MSG_CONFIRM for eva


#include "dns_receiver.h"
#include "../dns_packet.h"
#include "../dns_coding.h"
#include "dns_receiver_events.h"

void print_help(){
    printf("dns_receiver - represents a server application listening for DNS communication\n"
              "Usage: dns_receiver {BASE_HOST} {DST_DIRPATH}\n"
              "\t {BASE_HOST} - sets a base domain for incoming data\n"
              "\t {DST_DIRPATH} - DIRpath where will be stored all of accepted data/file\n");
}

int parse_args(int argc, char* argv[], Rec_args* args){
    if (argc != 3){
        fprintf(stderr, "Error: Incorrect input\n");
        print_help();
        return INPUT_ERR;
    }
    if (strlen(argv[1]) > sizeof(args->base_host)){
        fprintf(stderr, "Error: Too long base host, can't parse\n");
        print_help();
        return INPUT_ERR;
    }
    strcpy(args->base_host, argv[1]);

    if (strlen(argv[2]) > sizeof(args->dir_path)){
        fprintf(stderr, "Error: Too long dir path, can't parse\n");
        print_help();
        return INPUT_ERR;
    }
    strcpy(args->dir_path, argv[2]);
    return 0;
}

int get_filepath(char* packet, char* filepath){
    if (packet == NULL || filepath == NULL) {
        fprintf(stderr, "Failure while getting filepath\n");
        exit(FAILURE);
    }

    int read = 0, chunk_len, useless = 0;
    bool cycle = true;

    char* decode = malloc(NAME_SIZE);   // decoded filepath
    char chunk[NAME_SIZE];              // string for label
    char tmp[2];                        // temporary for adding chars to string

    if (decode == NULL){
        fprintf(stderr, "Malloc failure\n");
        exit(FAILURE);
    }

    tmp[1] = '\0';              // set end of the string
    filepath[0] = '\0';         // clear string

    while (cycle){
        chunk_len = (int) (packet[read]);         // get length of label
        chunk[0] = '\0';                          // clear string

        for (int i = 1; i <= chunk_len; i++){       // get chunk
            if (packet[read + i] == '-'){           // found end of the filepath
                cycle = false;
            }
            else{
                tmp[0] = packet[read + i];
                strcat(chunk, tmp);
            }
        }

        decode_string(chunk, (uint8_t **) &decode, &useless);
        decode[useless] = '\0';           // add ned of the string
        strcat(filepath, decode);
        read += chunk_len + 1;
        if (read > strlen(packet)){
            break;
        }
    }

    free(decode);
    return read;
}

bool check_domain(char* packet_start, char* domain, int* read){
    int chunk_len = 0, pack_len = (int)strlen(packet_start);
    bool corr = true;

    if (strlen(domain) + 1 != pack_len){        // lengths of domains isn't the same
        *read += pack_len;
        return false;
    }

    for (int i = 0; i < pack_len; i++){         // read till the end
        if (chunk_len == 0){                    // get length of the label
            if (i != 0){                        // not first label
                if (*domain != '.'){
                    corr = false;
                    break;
                }
                else{
                    domain++;                         // move in domain
                }
            }

            chunk_len = (int) *packet_start;
            if (chunk_len == 0){                      // the end of the packet
                break;
            }

            packet_start++;                           // move in packet
        }
        else if (*packet_start != *domain){           // found the difference
            corr = false;
            break;
        }
        else{
            packet_start++;                         // move by one in packet
            domain++;                               // move by one in domain
            chunk_len--;
        }
    }

    *read += pack_len;          // set read
    return corr;
}

void create_recursive_dir(char* dirpath){
    char* split;
    if (strchr(dirpath, '/')){                  // if dirpath have subdirectories
        split = strrchr(dirpath, '/');
        *split = '\0';
        create_recursive_dir(dirpath);
        *split = '/';
    }
    if (strcmp(dirpath, ".") == 0){
        return;
    }
    struct stat st;
    if (stat(dirpath, &st) == -1) {             // if dir doesn't exist
        mkdir(dirpath, 0700);
    }
};

void save_data(char* dirpath, char* filepath, uint8_t* data, int data_len, bool append){
    char path[strlen(dirpath) + strlen(filepath) + 1];      // filepath
    char tmp[2];                // temporary for adding char to string
    tmp[1] = '\0';
    strcpy(path, dirpath);

    if (dirpath[strlen(dirpath) - 1] != '/'){       // add '/' to the end of the dirpath
        tmp[0] = '/';
        strcat(path, tmp);
    }

    while(filepath[0] == '.' || filepath[0] == '/'){
        filepath++;                 // skip char
    }

    char* file;                     // split file from directories
    if (strchr(filepath, '/')){
        file = strrchr(filepath, '/');
    }
    else{
        file = filepath;
    }

    int len = strlen(filepath) - strlen(file);
    strncat(path, filepath, len);
    create_recursive_dir(path);         // create dirpath

    strcat(path, file);

    FILE* fileptr;

    if (append){
        fileptr = fopen(path, "ab");
    }
    else{
        fileptr = fopen(path, "wb");
    }

    if (fileptr == NULL){
        fprintf(stderr, "Couldn't open a file to save data\n");
        return;
    }

    fwrite(data, data_len, 1, fileptr);
    fclose(fileptr);
}

int parse_packet(char* packet, char* filepath, Rec_args* args, int* length, struct in_addr *src, int* file_len){
    dns_header* in_header = (dns_header*) packet;

    bool is_truncated = ntohs(in_header->flags) & SEND_TRANC_FLAG;       // check if the packet is truncated
    bool add = true;                    // flag if the file is being writen for the first time
    bool corr_domain = false;
    bool err = false;

    char* name = &(packet[sizeof(dns_header)]);

    char chunk[NAME_SIZE];          // string for label
    uint8_t* decoded = malloc(NAME_SIZE*CODE_SIZE/BYTE_SIZE);       // decoded label
    uint8_t* decoded_data = malloc(NAME_SIZE);
    char tmp[2];                    // temporary for adding char to string

    int data_len = 0;               // length of decoded_data
    int read = 0;                   // number of chars read
    int chunk_len;
    int decoded_len = 0;            // length of returned decoded data

    if (strlen(filepath) == 0){     // first access to the file
        read = get_filepath(&packet[sizeof(dns_header)], filepath);
        add = 0;
    }

    while (read < strlen(name)){
        chunk_len = (int) (name[read]);
        if (chunk_len == 0){        // end of question
            break;
        }
        else if (chunk_len > 63){   // wrong format of question
            err = true;
            break;
        }

        chunk[0] = '\0';            // clear string for label
        for (int i = 1; i <= chunk_len; i++){   // get label
            tmp[0] = name[read + i];
            strcat(chunk, tmp);
        }

        if (strstr(args->base_host, chunk) != NULL){    // check for base host
            corr_domain = check_domain(&name[read], args->base_host, &read);
            break;
        }
        else{
            dns_receiver__on_query_parsed(filepath, chunk);
            decode_string(chunk, &decoded, &decoded_len);
            memcpy(&decoded_data[data_len], decoded, decoded_len);
            data_len += decoded_len;
            decoded_len = 0;
        }
        read += chunk_len + 1;
    }
    *length = read + 1;     // set length of question name

    free(decoded);

    if (err){                  // error occurred in packet
        filepath[0] = '\0';         // clear filepath
        free(decoded_data);
        return RET_ERR;
    }
    else if (!corr_domain){              // incorrect domain, ignore packet
        free(decoded_data);
        return RET_SKIP;
    }
    else if (is_truncated){         // truncated packet
        dns_receiver__on_chunk_received(src, filepath, ntohs(in_header->xid), data_len);
        *file_len += data_len;
        if (data_len){              // there were some data
            save_data(args->dir_path, filepath, decoded_data, data_len, add);
        }
        free(decoded_data);
        return RET_TRUNC;
    }
    else{
        dns_receiver__on_chunk_received(src, filepath, in_header->xid, data_len);
        *file_len += data_len;
        if (data_len){              // somedata
            save_data(args->dir_path, filepath, decoded_data, data_len, add);
        }
        free(decoded_data);
        return RET_NORM;
    }
}

/**
 * Využietie zdrojov z nasledujúcej stránky pre implementáciu serverovej časti udp spojenia.
 * Všetky práva su vyhradené danému autorovi.
 * https://www.geeksforgeeks.org/udp-server-client-implementation-c/
 */
void prepare_udp_sock(int* udp_socket, struct sockaddr_in* rec_udp_addr){
    if ((*udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        fprintf(stderr, "Error: Couldn't create socket\n");
        exit(FAILURE);
    }

    rec_udp_addr->sin_family = AF_INET;
    rec_udp_addr->sin_addr.s_addr = INADDR_ANY;
    rec_udp_addr->sin_port = htons(DNS_PORT);

    if (bind(*udp_socket, (const struct sockaddr*) rec_udp_addr, sizeof(*rec_udp_addr)) == -1){
        fprintf(stderr, "Error: Couldn't bind socket to the port\n");
        exit (FAILURE);
    }
}

/**
 * Využietie zdrojov z nasledujúcej stránky pre implementáciu serverovej časti tcp spojenia.
 * Všetky práva su vyhradené danému autorovi.
 * https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
 */
int prepare_tcp_sock(int* tcp_socket, int* accept_tcp_sock, struct sockaddr_in* rec_tcp_addr, struct sockaddr_in* send_tcp_addr){
    if ((*tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        fprintf(stderr, "Error: Couldn't create socket\n");
        exit(FAILURE);
    }

    int optval = 1;
    if (setsockopt(*tcp_socket, SOL_SOCKET, SO_REUSEADDR, (const void*) &optval, sizeof(int)) < 0){
        fprintf(stderr, "Error: Couldn't set option to socket\n");
        exit(FAILURE);
    }

    rec_tcp_addr->sin_family = AF_INET;
    rec_tcp_addr->sin_addr.s_addr = INADDR_ANY;
    rec_tcp_addr->sin_port = htons(DNS_PORT);

    if (bind(*tcp_socket, (const struct sockaddr*) rec_tcp_addr, sizeof(*rec_tcp_addr)) == -1){
        fprintf(stderr, "Error: Couldn't bind socket to the port\n");
        exit (FAILURE);
    }

    if ((listen(*tcp_socket, 1)) == -1) {
        return -1;
    }

    socklen_t len = sizeof(*send_tcp_addr);

    if ((*accept_tcp_sock = accept(*tcp_socket, (struct sockaddr*) send_tcp_addr, &len)) == -1) {
        fprintf(stderr, "Error: Server accept failed\n");
        exit (FAILURE);
    }

    return 0;
}

/**
 * Function to handle the Ctrl + C kill signal.
 * */
void sighandle(){
    printf("Forced shut down of server\n");
    exit(SHUT_DOWN);
}


int main(int argc, char* argv[]){
    signal(SIGINT, sighandle);
    Rec_args args;
    int tmp = parse_args(argc, argv, &args);
    if (tmp){
        return tmp;
    }

    srand(time(NULL));

    int q_name_len = 0;                 // question name length
    int file_len = 0;
    char file_path[NAME_SIZE];          // transmitted filepath
    file_path[0] = '\0';                // clear string

    int udp_socket, tcp_socket, accept_tcp_sock;
    struct sockaddr_in rec_udp_addr, send_udp_addr, rec_tcp_addr, send_tcp_addr;
    socklen_t len = sizeof(send_udp_addr);

    prepare_udp_sock(&udp_socket, &rec_udp_addr);

    char* packet = malloc(DNS_SIZE);      // received dns packet
    long got;                             // length of data received

    while (1){
        got = recvfrom(udp_socket, packet, DNS_SIZE, 0, (struct sockaddr *)(&send_udp_addr), &len);
        if (got == -1){
            fprintf(stderr, "Error: Unsuccessful read of datagram\n");
            close(udp_socket);
            return FAILURE;
        }
        if (got == 0){
            break;
        }

        dns_receiver__on_transfer_init(&send_udp_addr.sin_addr);

        // parse data
        int flag = parse_packet(packet, file_path, &args, &q_name_len, &send_udp_addr.sin_addr, &file_len);

        if (flag == RET_TRUNC){                 // return truncated response and prepare for receiving data on tcp
            dns_header* new_header = (dns_header*) packet;
            set_flags_header(new_header, REC_NORM_FLAG);

            sendto(udp_socket, packet, sizeof(dns_header) + q_name_len + sizeof(dns_question), MSG_CONFIR, (struct sockaddr *)(&send_udp_addr),
                   len);

            prepare_tcp_sock(&tcp_socket, &accept_tcp_sock, &rec_tcp_addr, &send_tcp_addr);

            while(read(accept_tcp_sock, packet, DNS_SIZE)){         // read tcp queries and respond
                parse_packet(&packet[sizeof(uint16_t)], file_path, &args, &q_name_len, &send_udp_addr.sin_addr, &file_len);

                new_header = (dns_header*) &packet[sizeof(uint16_t)];
                set_flags_header(new_header, REC_NORM_FLAG);

                write(accept_tcp_sock, packet, sizeof(dns_header) + sizeof(uint16_t) + q_name_len + sizeof(dns_question));
            }

            close(tcp_socket);
            close(accept_tcp_sock);
        }
        else if (flag == RET_NORM){                             // only return an udp response
            dns_header* new_header = (dns_header*) packet;
            set_flags_header(new_header, REC_NORM_FLAG);

            sendto(udp_socket, packet, sizeof(dns_header) + q_name_len + sizeof(dns_question), MSG_CONFIR, (struct sockaddr *)(&send_udp_addr),
                   len);
        }
        else if (flag == RET_ERR){                              // return an error dns packet
            dns_header* new_header = (dns_header*) packet;
            set_flags_header(new_header, REC_ERR_FLAG);
            set_questions_header(new_header, 0);

            sendto(udp_socket, packet, sizeof(dns_header), MSG_CONFIR, (struct sockaddr *)(&send_udp_addr),
                   len);
        }
        else {      // RET_SKIP
            ;       // received packet didn't match the base_host, don't respond
        }
        dns_receiver__on_transfer_completed(file_path, file_len);
        file_path[0] = '\0';         // clear filepath
        file_len = 0;
    }

    free(packet);
}