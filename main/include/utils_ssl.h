#ifndef UTILS_SSL_H_
#define UTILS_SSL_H_

#include "sys_header.h"

void init_openssl();

SSL_CTX* create_ssl_context();

void cleanup_openssl();

#endif