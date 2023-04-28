/**
* Project name: Tunelování datových přenosů přes DNS dotazy
* Author: Veronika Molnárová
* Date: 6.11 2022
* Subject: Síťové aplikace a správa sítí
*/

#ifndef PROJECT_DNS_CODING_H
#define PROJECT_DNS_CODING_H

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      /// Checking for upper/lower characters

#define CODE_ERR -5     /// Error while coding
#define CODE_SIZE 5     /// Coding into base32, using 5b
#define BYTE_SIZE 8     /// Bits in byte
#define UPPER_CASE_TILL 26      /// 26 uppercase letter for indexes 0-25
#define UPPER_CASE_CONST 65     /// Const needed to be added to uppercase to get the actual letter
#define LOWER_CASE_TILL 32      /// Rest of the characters indexes 26-31 are lowercase letters
#define LOWER_CASE_CONST 71     /// Const needed to be added to lowercase get the actual letter

/// Encodes the passed array with length 'in_len' into base32 returned in output
void encode_string(u_int8_t* in, int in_len, char** out);

/// Decodes the passed string from base32 into array returned in output with its length 'out_len'
void decode_string(char* in, u_int8_t** out, int* out_len);


#endif //PROJECT_DNS_CODING_H
