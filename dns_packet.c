/**
* Project name: Tunelování datových přenosů přes DNS dotazy
* Author: Veronika Molnárová
* Date: 6.11 2022
* Subject: Síťové aplikace a správa sítí
*/

#include "dns_packet.h"

void prepare_header(dns_header* header, unsigned short id){
    header->xid = htons(id);    //using srand() before creating the header
    header->flags = 0;
    header->qdcount = htons(1);
    header->ancount = 0;
    header->nscount = 0;
    header->arcount = 0;
}

void set_flags_header(dns_header* header, unsigned int set){
    header->flags = header->flags | (unsigned int)htons(set);
}

void set_questions_header(dns_header* header, unsigned int set){
    header->qdcount = htons(set);
}


void debug_header(dns_header* header) {
    printf("------------HEADER------------\n");
    printf("ID: %d\n", ntohs(header->xid));
    printf("%x\n", ntohs(header->flags));
    printf("%d\n", ntohs(header->qdcount));
    printf("%d\n", ntohs(header->ancount));
    printf("%d\n", ntohs(header->nscount));        // unused
    printf("%d\n", ntohs(header->arcount));        // unused
}

void set_question(dns_question* question, unsigned short type, unsigned short clas){
    question->q_type = htons(type);
    question->q_class = htons(clas);
}

void debug_question(dns_question* question) {
    printf("------------QUESTION------------\n");
    printf("%x\n", ntohs(question->q_type));
    printf("%x\n", ntohs(question->q_class));
}