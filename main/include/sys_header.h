#ifndef SYS_HEADER_
#define SYS_HEADER_

/**
 * @file sys_header.h
 * @brief Inlcude all C/C++ header files for convenience
 * @version 0.1
 * @date 2024-05-14
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>
#include <stddef.h>
#include <strings.h>
#include <ctype.h>

#include <arpa/inet.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/time.h>

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/x509_vfy.h>

#endif