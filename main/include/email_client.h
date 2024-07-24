#ifndef EMAIL_CLIENT_H
#define EMAIL_CLIENT_H

#include "sys_header.h"

// Port number for the IMAP server
#define PORT 993

// Buffer size for reading from the socket
#define BUFFER_SIZE 1024

/**
 * @brief Structure to hold email client configuration
 * 
 */
typedef struct email_client {
    char *username;
    char *password;
    char *folder;
    char *message_num;
    char *command;
    char *server_name;
    int tls;
} email_client_t;

void init_config(email_client_t *config);

int parse_command_line(int argc, char *argv[], email_client_t *config);

#endif
