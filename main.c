#include <stdio.h>
#include <string.h>
#include "dns_coding.h"

//01001 00001 10010 10110 11000 11011 00011 01111
//01001 00001 10010 10110 11000 11011 00011 01111

int main() {
    char* heh = malloc(256);
    char* code = malloc(256);
    char* decode = malloc(256);
    strcpy(heh, "hello");
    encode_string(heh, &code);
    printf("Coded: \n%s\n", code);
    decode_string(code, &decode);
    printf("Decoded: \n%s\n", decode);
    return 0;
}
