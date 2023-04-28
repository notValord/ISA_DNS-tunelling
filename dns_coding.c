/**
* Project name: Tunelování datových přenosů přes DNS dotazy
* Author: Veronika Molnárová
* Date: 6.11 2022
* Subject: Síťové aplikace a správa sítí
*/

#include "dns_coding.h"

void encode_string(u_int8_t* in, int in_len, char** out){
    if (in == NULL || out == NULL){
        fprintf(stderr, "Can't encode data\n");
        exit(CODE_ERR);
    }

    char binary[in_len * BYTE_SIZE + 1];    // binary input
    char tmp[2];                        // temporary for adding chars into string
    tmp[1] = '\0';                      // set end of the string
    binary[0] = '\0';                   // clear string

    for (unsigned int i = 0; i < in_len; i++){                  // get char one by one
        for (int j = 1 << (BYTE_SIZE - 1); j > 0; j = j / 2){   // turn to binary
            if (in[i] & j) {
                tmp[0] = '1';
            }
            else{
                tmp[0] = '0';
            }
            strcat(binary, tmp);
        }
    }
    in_len = (int)strlen(binary);

    //padding the input with 0 to be dividable by 5
    int padding = CODE_SIZE - (in_len%CODE_SIZE);
    char new_in[in_len + padding];                  // binary with padding
    strcpy(new_in, binary);

    if (padding < CODE_SIZE){
        for (int i = 0; i < padding; i++){
            strcat(new_in, "0");
        }
    }
    else{
        padding = 0;
    }
    in_len += padding;                              // update length

    unsigned int dec_arr[in_len/CODE_SIZE];         // code each 5b to decimals
    char sub_str[CODE_SIZE + 1];                    // 5b substring + '\0'

    for (int i = 0; i < in_len/CODE_SIZE; i++){
        strncpy(sub_str, &new_in[i*CODE_SIZE], CODE_SIZE);
        sub_str[CODE_SIZE] = '\0';
        dec_arr[i] = strtoull(sub_str, NULL, 2);    // from binary to decimal
    }

    (*out)[0] = '\0';   //clear output
    for (int i = 0; i < in_len/CODE_SIZE; i++){
        if (dec_arr[i] < UPPER_CASE_TILL){                      // code as an uppercase letter
            tmp[0] = (char) (dec_arr[i] + UPPER_CASE_CONST);
        }
        else if (dec_arr[i] < LOWER_CASE_TILL) {                // code as a lowercase letter
            tmp[0] = (char) (dec_arr[i] + LOWER_CASE_CONST);
        }
        else {                                                  // not being able to code to base32
            fprintf(stderr, "Out of range coding\n");
            exit(CODE_ERR);
        }
        strcat(*out, tmp);
    }
}

void decode_string(char* in, u_int8_t** out, int* out_len){
    if (in == NULL || out == NULL || out_len == NULL){
        fprintf(stderr, "Can't decode data\n");
        exit(CODE_ERR);
    }

    int in_size = (int)strlen(in);

    unsigned int dec_arr[in_size];          // decimal array of input
    char binary[in_size*CODE_SIZE + 1];         // array for binary
    binary[0] = '\0';   //clear string

    int over_padding = (in_size*CODE_SIZE)%BYTE_SIZE;   // get the padding that was added when encoding

    for (int index = 0; index < in_size; index++){      // get decimal representation of binary code
        if (isupper(in[index])){                        // uppercase
            dec_arr[index] = (in[index] - UPPER_CASE_CONST);
        }
        else if (islower(in[index])){                   // lowercase
            dec_arr[index] = (in[index] - LOWER_CASE_CONST);
        }
        else{                                              // unknown character, can't decode the message
            fprintf(stderr, "Out of range coding");
            return;
        }
    }


    char tmp[2];                // temporary for adding chars to string
    tmp[1] = '\0';              // set end of the string
    *out_len = 0;

    for (int i = 0; i < in_size; i++){
        unsigned int to_bin = dec_arr[i];
        for (int j = 1 << (CODE_SIZE - 1); j > 0; j = j / 2){   // decimal to binary
            if (to_bin & j) {
                tmp[0] = '1';
            }
            else{
                tmp[0] = '0';
            }
            strcat(binary, tmp);
        }
    }

    binary[strlen(binary) - over_padding] = '\0';   // cut off the overpadding of '0' from the back

    char sub_str[BYTE_SIZE + 1];    // substring for decoded char 8b + '\0'
    for (int i = 0; i < strlen(binary)/BYTE_SIZE; i++){
        strncpy(sub_str, &binary[i*BYTE_SIZE], BYTE_SIZE);
        sub_str[BYTE_SIZE] = '\0';
        (*out)[(*out_len)++] = strtoull(sub_str, NULL, 2);    //binary to decimal
    }
}