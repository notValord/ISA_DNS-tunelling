/**
* Project name: Tunelování datových přenosů přes DNS dotazy
* Author: Veronika Molnárová
* Date: 6.11 2022
* Subject: Síťové aplikace a správa sítí
*/

#define MSG_CONFIR 2048         // const for MSG_CONFIRM for eva

#include "dns_sender.h"
#include "../dns_coding.h"
#include "../dns_packet.h"
#include "dns_sender_events.h"

void print_help(){
    printf("dns_sender - represents a client application sending data to DNS server\n"
              "Usage: dns_sender [-u UPSTREAM_DNS_IP] {BASE_HOST} {DST_FILEPATH} [SRC_FILEPATH]\n"
              "\t -u - sets a remote DNS server\n"
              "\t    - if not specified, program will use the default DNS server set in the system\n"
              "\t {BASE_HOST} - sets a base domain for all transitions\n"
              "\t {DST_FILEPATH} - filepath where the data will be stored on the DNS server\n"
              "\t [SRC_FILEPATH] - path to the input file which will be sent\n"
              "\t                - if not specified, program will read the data from the STDIN\n");
}

int parse_args(int argc, char* argv[], Send_args* args){
    if (argc > 6 || argc < 3){          // wrong number of arguments
        fprintf(stderr, "Error: Incorrect input\n");
        print_help();
        return INPUT_ERR;
    }

    int state = 0;      // state to keep track of positional arguments
    bool ip = false;    // checks whether the ip was input
    strcpy(args->input, STDIN);     // set default to STDIN

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-u") == 0){
            if (ip || i+1 >= argc){     // double of ips or not inserted at all
                fprintf(stderr, "Error: Incorrect input\n");
                print_help();
                return INPUT_ERR;
            }

            if (strlen(argv[i+1]) > sizeof(args->dns_ip)){
                fprintf(stderr, "Error: Too long IP address, taking only IPv4\n");
                print_help();
                return INPUT_ERR;
            }
            if (inet_addr(argv[i+1]) == -1){
                fprintf(stderr, "Error: Isn't an IPv4 address\n");
                print_help();
                return INPUT_ERR;
            }

            strcpy(args->dns_ip, argv[i+1]);
            ip = true;
            i++;            // skip the next argument
            continue;
        }

        switch (state) {
            case 0:
                if (strlen(argv[i]) > sizeof(args->base_host)){
                    fprintf(stderr, "Error: Too long base host, can't parse\n");
                    return INPUT_ERR;
                }
                strcpy(args->base_host, argv[i]);
                break;

            case 1:
                if (strlen(argv[i]) > sizeof(args->file_path)){
                    fprintf(stderr, "Error: Too long file path, can't parse\n");
                    return INPUT_ERR;
                }
                strcpy(args->file_path, argv[i]);
                break;

            case 2:
                if (strlen(argv[i]) > sizeof(args->base_host)){
                    fprintf(stderr, "Error: Too long input file, can't parse\n");
                    return INPUT_ERR;
                }
                strcpy(args->input, argv[i]);
                break;

            default:
                fprintf(stderr, "Error: Incorrect input\n");
                print_help();
                return INPUT_ERR;
        }
        state++;
    }

    if (state < 2) {                // didn't read enough mandatory arguments
        fprintf(stderr, "Error: Incorrect input\n");
        print_help();
        return INPUT_ERR;
    }

    if (!ip){                       // getting the default dns server from resolv.conf file
        FILE* conf_file = fopen(CONF_FILE, "r");
        if (conf_file == NULL){
            fprintf(stderr, "Couldn't open resolv.conf file to get the default DNS server\n");
            exit(FILE_ERR);
        }

        char* line = NULL;
        size_t line_max = MAX_DOMAIN_LEN;

        while (getline(&line, &line_max, conf_file) != -1){
            if (line[0] == '#'){    //comment
                continue;
            }

            if (strstr(line, "nameserver") != NULL){
                char* token = strrchr(line, ' ');   // get the last word from the line
                if (token == NULL){
                    token = strrchr(line, '\t');
                    if (token == NULL) {
                        fprintf(stderr, "Couldn't get the nameserver\n");
                        exit(FILE_ERR);
                    }
                }
                strcpy(args->dns_ip, &token[1]);
                break;
            }
        }
        fclose(conf_file);
    }

    return 0;
}

void debug_args(Send_args* args){
    printf("IP: %s\n", args->dns_ip);
    printf("Input: %s\n", args->input);
    printf("Dest file: %s\n", args->file_path);
    printf("Base host: %s\n", args->base_host);
}


void prepare_domain(char* encoded_data, const char* domain){
    if (encoded_data == NULL || domain == NULL) {
        fprintf(stderr, "Failure while preparing domain name\n");
        exit(FAILURE);
    }

    char token[MAX_DOMAIN_LEN]; // dns format of domain
    char tmp[2];                // temporary for adding char to string
    tmp[1] = '\0';              // set end of the string
    token[0] = '\0';            // clear string
    encoded_data[0] = '\0';     // clear string

    for (int i = 0; i < strlen(domain); i++){
        if (domain[i] == '.'){                  // changes '.' for the length of the label
            tmp[0] = (char)(strlen(token));     // get length of the label
            strcat(encoded_data, tmp);
            strcat(encoded_data, token);
            token[0] = '\0';                    // clear string
        }
        else {
            tmp[0] = domain[i];                 // add char to string
            strcat(token, tmp);
        }
    }

    if (strlen(token)){                         // for the last label if something was left
        tmp[0] = (char) (strlen(token));
        strcat(encoded_data, tmp);
        strcat(encoded_data, token);
    }
}

void prepare_file_path(char* encoded_path, const char* path){
    if (encoded_path == NULL || path == NULL) {
        fprintf(stderr, "Failure while preparing filepath\n");
        exit(FAILURE);
    }

    unsigned int len_path = strlen(path);
    char substr[MAX_CHARS_FOR_CHUNK + 1];           // substring which fits into a single label
    char* coded = malloc(MAX_CHUNK + 1);        // substring for coded label
    char tmp[2];                                    // temporary for adding char to string

    if (coded == NULL){
        fprintf(stderr, "Malloc failure\n");
        exit(FAILURE);
    }

    // clear strings
    encoded_path[0] = '\0';
    substr[0] =  '\0';
    coded[0] = '\0';
    tmp[1] = '\0';          // set end of the string

    // ((len_path + (MAX_CHARS_FOR_CHUNK - 1) )/ MAX_CHARS_FOR_CHUNK) ==> number of labels needed for the path
    for (unsigned int i = 0; i < ((len_path + (MAX_CHARS_FOR_CHUNK - 1) )/ MAX_CHARS_FOR_CHUNK); i++){   //get char one by one
        strncpy(substr, &path[i*MAX_CHARS_FOR_CHUNK], MAX_CHARS_FOR_CHUNK);

        encode_string((uint8_t*)substr, (int)strlen(substr), &coded);   // encode data
        tmp[0] = (char) strlen(coded);                                  // length of the label in hex

        if (i == ((len_path + (MAX_CHARS_FOR_CHUNK - 1) )/ MAX_CHARS_FOR_CHUNK) - 1){   // last label
            if (tmp[0] == MAX_CHARS_FOR_CHUNK){     // max label
                strcat(encoded_path, tmp);
                strcat(encoded_path, coded);

                tmp[0] = (char) 1;          // create a new label with single char '-' to mark the end of the filepath
                strcat(encoded_path, tmp);
                strcat(encoded_path, "-");
            }
            else{
                tmp[0]++;                   // add '-' to the end of the label
                strcat(encoded_path, tmp);
                strcat(encoded_path, coded);
                strcat(encoded_path, "-");
            }
        }
        else{
            strcat(encoded_path, tmp);
            strcat(encoded_path, coded);

            coded[0] = '\0';
        }
    }

    free(coded);
}

int read_file(char* encoded_data, FILE* file, int max_send, bool std, int* chunk_size, char* filepath, unsigned short id){
    if (file == NULL && !std){
        fprintf(stderr, "Can't read input file\n");
        exit(FILE_ERR);
    }

    int truct = 0;          // flag whether the question will be truncated
    int ok_read;            // checker for successful read from file
    uint8_t c;              // char to read

    // (max_read /(MAX_CHUNK + 1)) * MAX_CHARS_FOR_CHUNK ==> number of full labels * max chars for label
    // ((max_send /(MAX_CHUNK + 1)) * MAX_CHUNK) ==> number of coded chars in full labels
    // ((max_read + MAX_CHUNK)/(MAX_CHUNK + 1)) ==> number of labels
    // (max_read - ((max_send /(MAX_CHUNK + 1)) * MAX_CHUNK) - ((max_read + MAX_CHUNK)/(MAX_CHUNK + 1))) * CODE_SIZE / BYTE_SIZE ==> max chars to read for the last label

    int max_get = (max_send /(MAX_CHUNK + 1)) * MAX_CHARS_FOR_CHUNK;         // max number of chars to read from file
    max_get += (max_send - ((max_send /(MAX_CHUNK + 1)) * MAX_CHUNK) - ((max_send + MAX_CHUNK)/(MAX_CHUNK + 1))) * CODE_SIZE / BYTE_SIZE;

    uint8_t substr[MAX_CHARS_FOR_CHUNK + 1];        // substring which fits into a single label
    int substr_cnt = 0;                             // length of substr

    char* coded = malloc(MAX_CHARS_FOR_CHUNK * BYTE_SIZE);      // coded data
    char tmp[2];                // temporary for adding char to string
    tmp[1] = '\0';              // set end of the string

    encoded_data[0] = '\0';     // clear string

    if (std){       // read from stdin
        ok_read = (int)fread(&c, 1, 1, stdin);
    }
    else{           // read file
        ok_read = (int)fread(&c, 1, 1, file);
    }

    while (ok_read == 1){
        substr[substr_cnt++] = c;       // add char to the substring
        (*chunk_size)++;

        max_get--;
        if (max_get == 0){                      // max size of dns question reached
            encode_string(substr, substr_cnt, &coded);      // encode data
            dns_sender__on_chunk_encoded(filepath, id, coded);

            tmp[0] = (char) strlen(coded);      // add length of the label as hex
            strcat(encoded_data, tmp);
            strcat(encoded_data, coded);

            truct = 1;                          // EOF not reached, needs to send multiple queries
            substr_cnt = 0;                     // reset counter
            break;
        }

        if (substr_cnt == MAX_CHARS_FOR_CHUNK){             // max number of characters for label read
            encode_string(substr, substr_cnt, &coded);      // encode data
            dns_sender__on_chunk_encoded(filepath, id, coded);

            tmp[0] = (char) strlen(coded);                  // add length of the label as hex
            strcat(encoded_data, tmp);
            strcat(encoded_data, coded);

            substr_cnt = 0;                                 // reset counter
            coded[0] = '\0';
        }

        if (std){               // read from stdin
            ok_read = (int)fread(&c, 1, 1, stdin);
        }
        else{                   // read file
            ok_read = (int)fread(&c, 1, 1, file);
        }
    }

    if (substr_cnt){                                // last label if something was left
        encode_string(substr, substr_cnt, &coded);  // encode data
        dns_sender__on_chunk_encoded(filepath, id, coded);

        tmp[0] = (char) strlen(coded);              // add length of the label as hex
        strcat(encoded_data, tmp);
        strcat(encoded_data, coded);
    }

    free(coded);
    return truct;
}

int build_packet(char* buffer, char* encoded_path, char* encoded_data, char* encoded_base, int trunct, unsigned short id){
    struct header* new_header = (struct header*) buffer;
    prepare_header(new_header, id);

    if (trunct){            // truncated packet
        set_flags_header(new_header, SEND_TRUNC_FLAG);
    }
    else{
        set_flags_header(new_header, SEND_FLAG);
    }

    char* q_name = &buffer[sizeof(dns_header)];
    if (strlen(encoded_path)){          // first packet with filepath
        strcpy(q_name, encoded_path);
        strcat(q_name, encoded_data);
        strcat(q_name, encoded_base);
    }
    else{                               // no need to send filepath again
        strcpy(q_name, encoded_data);
        strcat(q_name, encoded_base);
    }

    dns_question* new_question = (dns_question*) &(buffer[sizeof(dns_header) + strlen(q_name) + 1]);    // needs to add 1 for the zero character at the end
    set_question(new_question, SEND_TYPE, SEND_CLASS);

    return (int)strlen(q_name);
}

/**
 * Využietie zdrojov z nasledujúcej stránky pre implementáciu klientskej časti udp spojenia.
 * Všetky práva su vyhradené danému autorovi.
 * https://www.geeksforgeeks.org/udp-server-client-implementation-c/
 */
void prepare_udp_sock(int* udp_socket, struct sockaddr_in* server_addr, Send_args* args){
    if ((*udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        fprintf(stderr, "Error: Couldn't create socket\n");
        exit(FAILURE);
    }

    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = inet_addr(args->dns_ip);
    server_addr->sin_port = htons(DNS_PORT);
}

/**
 * Využietie zdrojov z nasledujúcej stránky pre implementáciu klientskej časti tcp spojenia.
 * Všetky práva su vyhradené danému autorovi.
 * https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
 */
void prepare_tcp_sock(int* tcp_socket, struct sockaddr_in* server_tcp_addr, Send_args* args){
    if ((*tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        fprintf(stderr, "Error: Couldn't create socket\n");
        exit(FAILURE);
    }

    server_tcp_addr->sin_family = AF_INET;
    server_tcp_addr->sin_addr.s_addr = inet_addr(args->dns_ip);
    server_tcp_addr->sin_port = htons(DNS_PORT);

    int i;
    for (i = 0; i < 500; i++){     // tries to connect to the server for a given time
        if (connect(*tcp_socket, (const struct sockaddr*) server_tcp_addr, sizeof(*server_tcp_addr)) == 0){
            break;
        }
        usleep(5);          // wait
    }

    if (i == 500){                  // didn't break
        fprintf(stderr, "Error: Couldn't connect socket\n");
        exit(FAILURE);
    }
}

int main(int argc, char* argv[]) {
    Send_args args;
    int tmp = parse_args(argc, argv, &args);
    if (tmp) {
        return tmp;
    }

    srand(time(0));

    char* buffer = (char*) malloc(DNS_SIZE);
    char encoded_base[MAX_DOMAIN_LEN];
    char encoded_path[MAX_DOMAIN_LEN];
    char encoded_data[MAX_DOMAIN_LEN];

    if (buffer == NULL){
        fprintf(stderr, "Malloc failure\n");
        exit(FAILURE);
    }

    prepare_file_path(encoded_path, args.file_path);
    prepare_domain(encoded_base, args.base_host);

    FILE* file = NULL;
    unsigned int rest = MAX_DOMAIN_LEN - strlen(encoded_path) - strlen(encoded_base) - 1;   // left space in question name for data
    int trunct;         // flag if the packet will be truncated
    int chunk_size = 0, file_size = 0;
    unsigned short xid = rand() % USHRT_MAX;

    int udp_socket;
    struct sockaddr_in server_addr;
    socklen_t len = sizeof(server_addr);
    prepare_udp_sock(&udp_socket, &server_addr, &args);

    //  initiation
    dns_sender__on_transfer_init(&server_addr.sin_addr);

    if (!strcmp(args.input, STDIN)){
        trunct = read_file(encoded_data, file, (int)rest, true, &chunk_size, args.file_path, xid);
    }
    else{
        file = fopen(args.input, "rb");
        trunct = read_file(encoded_data, file, (int)rest, false, &chunk_size, args.file_path, xid);
    }

    int q_name_len = build_packet(buffer, encoded_path, encoded_data, encoded_base, trunct, xid);

    // sending data
    dns_sender__on_chunk_sent(&server_addr.sin_addr, args.file_path, xid, chunk_size);

    file_size += chunk_size;
    chunk_size = 0;

    int cycle = 0;

    // timeout
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(udp_socket, &rset);
    do{
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        cycle++;

        // sends udp dns packet
        sendto(udp_socket, buffer, sizeof(dns_header) + q_name_len + 1 + sizeof(dns_question), MSG_CONFIR, (struct sockaddr *)&server_addr, len);

        int nready = select(udp_socket + 1, &rset, NULL, NULL, &timeout);

        if (nready > 0){
            recvfrom(udp_socket, buffer, DNS_SIZE, 0,(struct sockaddr *) &server_addr, &len);
            cycle = 0;
        }
        else if (nready < 0){
            fprintf(stderr, "Error with select\n");
            return FAILURE;
        }
        else{
            ; // do nothing
        }

        if (cycle == 5){
            fprintf(stderr, "Error: No respond from the server\n");
            return FAILURE;
        }
    } while (cycle);


    dns_header* rec_header = (dns_header*) buffer;

    // if the header is without errors and truncated
    if ((rec_header->flags & htons(0x0200)) && ((rec_header->flags & htons(0x000f)) == 0)){

        // initiate a tcp connection
        int tcp_socket;
        struct sockaddr_in server_tcp_addr;
        prepare_tcp_sock(&tcp_socket, &server_tcp_addr, &args);

        while(trunct){              // while packets are truncated(whole file wasn't send)
            rest = MAX_DOMAIN_LEN - strlen(encoded_base) - 1;       // sending without filepath
            xid = rand() % USHRT_MAX;

            if (!strcmp(args.input, STDIN)){        // send from stdin
                trunct = read_file(encoded_data, file, (int)rest, true, &chunk_size, args.file_path, xid);
            }
            else{                                   // send file
                trunct = read_file(encoded_data, file, (int)rest, false, &chunk_size, args.file_path, xid);
            }

            buffer[0] = '\0';                                       // clear buffer
            q_name_len = build_packet(&buffer[sizeof(uint16_t)], "", encoded_data, encoded_base, trunct, xid);

            uint16_t* tcp_len = (uint16_t*) buffer;                 // length of the dns query in first 2B of payload
            *tcp_len = htons((uint16_t)(sizeof(dns_header) + q_name_len + 1 + sizeof(dns_question)));

            dns_sender__on_chunk_sent(&server_addr.sin_addr, args.file_path, xid, chunk_size);

            file_size += chunk_size;
            chunk_size = 0;

            // send tcp dns packet
            write(tcp_socket, buffer, sizeof(dns_header) + q_name_len + 1 + sizeof(dns_question) + sizeof(uint16_t));
            read(tcp_socket, buffer, DNS_SIZE);
        }

        close(tcp_socket);
    }
    dns_sender__on_transfer_completed(args.file_path, file_size);

    close(udp_socket);

    if (file){
        fclose(file);
    }
    free(buffer);
    return 0;
}