#include "sys_header.h"
#include "email_client.h"
#include "imap.h"
#include "utils.h"
#include "utils_ssl.h"

/**
 * @brief Main Function to perform Fetch Mail 
 * 
 *
 * @param argc 
 * @param argv 
 * @return 
 */
int main(int argc, char **argv) {

    int sockfd, s;
    struct addrinfo hints, *servinfo, *rp;

    email_client_t *client = malloc(sizeof(email_client_t));
    if (client == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }
    init_config(client);

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <username> <password> <folder> <server>\n", argv[0]);
        free(client);
        exit(EXIT_FAILURE);
    }

    if (parse_command_line(argc, argv, client) != 0) {
        free(client);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    s = getaddrinfo(client->server_name, client->tls ? "993" : "143", &hints, &servinfo); // check port
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        free(client);
        exit(EXIT_FAILURE);
    }

    for (rp = servinfo; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) break;

        close(sockfd);
    }
    if (rp == NULL) {
        fprintf(stderr, "client: Failed to connect\n");
        freeaddrinfo(servinfo);
        free(client);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(servinfo);

    set_socket_timeout(sockfd, 10); // set socket time out 

    char *buffer = malloc(BUFFER_SIZE * 100);
    if (!buffer) {
        perror("Malloc failed.");
        exit(EXIT_FAILURE);
    }

    if (client->tls) {
        init_openssl();
        SSL_CTX *ctx = create_ssl_context();
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, sockfd);

        if (SSL_connect(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            cleanup_openssl();
            free(buffer);
            free(client);
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        imap_login_ssl(ssl, buffer, client->username, client->password);
        select_folder_ssl(ssl, buffer, client->folder);

        if (strcmp(client->command, "retrieve") == 0) {
            fetch_mail_ssl(ssl, buffer, client->message_num);
        } else if (strcmp(client->command, "parse") == 0) {
            parse_mail_ssl(ssl, buffer, client->message_num);
        } else if (strcmp(client->command, "mime") == 0) {
            fetch_mail_mime_ssl(ssl, buffer, client->message_num);
        } else if (strcmp(client->command, "list") == 0) {
            subjects_list_ssl(ssl, buffer);
        }

        imap_logout_ssl(ssl, buffer);

        SSL_free(ssl);
        SSL_CTX_free(ctx);
        cleanup_openssl();

    } else {
        imap_login(sockfd, buffer, client->username, client->password);
        select_folder(sockfd, buffer, client->folder);

        if (strcmp(client->command, "retrieve") == 0) {
            fetch_mail(sockfd, buffer, client->message_num);
        } else if (strcmp(client->command, "parse") == 0) {
            parse_mail(sockfd, buffer, client->message_num);
        } else if (strcmp(client->command, "mime") == 0) {
            fetch_mail_mime(sockfd, buffer, client->message_num);
        } else if (strcmp(client->command, "list") == 0) {
            subjects_list(sockfd, buffer);
        }

        imap_logout(sockfd, buffer);
    }

    free(buffer);
    free(client);
    close(sockfd);
    
    return 0;
}
