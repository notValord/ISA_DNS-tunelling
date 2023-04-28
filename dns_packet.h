/**
* Project name: Tunelování datových přenosů přes DNS dotazy
* Author: Veronika Molnárová
* Date: 6.11 2022
* Subject: Síťové aplikace a správa sítí
*/

#ifndef PROJECT_DNS_PACKET_H
#define PROJECT_DNS_PACKET_H

#include <limits.h>         /// used to get USHRT_MAX for xid
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>      /// htons(), ntohs()

/**
 * Vytvorenie vlastných štruktúr na základe zdrojov z nasledujúcej stránky s danými licenciami:
 * https://opensource.apple.com/source/netinfo/netinfo-208/common/dns.h.auto.html
 *
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 *
 * @APPLE_LICENSE_HEADER_END@
 *
 *
 *
 * Thread-safe DNS client library
 *
 * Copyright (c) 1998 Apple Computer Inc.  All Rights Reserved.
 * Written by Marc Majka
 *
 *
 */
/// DNS header structure
typedef struct header {
    unsigned short xid;         /// unique id
    unsigned short flags;
    unsigned short qdcount;     /// number of questions
    unsigned short ancount;     /// number of answers
    unsigned short nscount;
    unsigned short arcount;
}dns_header;

/// Initiates the header with the default data
void prepare_header(dns_header* header, unsigned short id);

/// Sets flags of the header to 'set' while others remain untouched
void set_flags_header(dns_header* header, unsigned int set);

/// Sets number of questions in the header to number 'set'
void set_questions_header(dns_header* header, unsigned int set);

/// Prints data in the header
void debug_header(dns_header* header);

/// DNS question structure
typedef struct question {
    unsigned short q_type;
    unsigned short q_class;
}dns_question;

/// Sets the question data to 'type' and 'clas'
void set_question (dns_question* question, unsigned short type, unsigned short clas);

/// Prints data in the header
void debug_question(dns_question* question);

#endif //PROJECT_DNS_PACKET_H
