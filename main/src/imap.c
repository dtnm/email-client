#include "sys_header.h"
#include "email_client.h"
#include "utils.h"
#include "utils_ssl.h"

/**
 * @brief Perform IMAP login with the specified username and password

 *
 * @param sockfd
 * @param buffer
 * @param username
 * @param password
 */
void imap_login(int sockfd, char *buffer, const char *username, const char *password)
{
    snprintf(buffer, BUFFER_SIZE, "%s LOGIN %s %s\r\n", generate_tag(), username, password);

    // Send command to connect to IMAP server
    if (send(sockfd, buffer, strlen(buffer), 0) < 0)
    {
        fprintf(stderr, "Failed to send login command\n");
        exit(3);
    }
    // Reading server response
    char response[BUFFER_SIZE] = {0};
    size_t response_length = 0;
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t nbytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (nbytes <= 0)
        {
            fprintf(stderr, "Server closed connection unexpectedly\n");
            close(sockfd);
            exit(3);
        }
        strncat(response, buffer, nbytes);
        response_length += nbytes;

        // Check if IMAP server establish was successful
        if (strstr(buffer, "OK") != NULL)
        {
            // Check log in
            if (strstr(response, "Logged in") != NULL)
            {
                break;
            }
        }
        else if (strstr(buffer, "NO") != NULL)
        {
            printf("Login failure\n");
            close(sockfd);
            exit(3);
        }
        else if (strstr(buffer, "BAD") != NULL)
        {
            printf("Login failure\n");
            close(sockfd);
            exit(1);
        }
    }
}

/**
 * @brief Perform IMAP logout
 *
 * @param sockfd
 * @param buffer
 */
void imap_logout(int sockfd, char *buffer)
{
    snprintf(buffer, BUFFER_SIZE, "%s LOGOUT\r\n", generate_tag());

    if (send(sockfd, buffer, strlen(buffer), 0) < 0)
    {
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t nbytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (nbytes <= 0)
        {
            close(sockfd);
            exit(3);
        }

        if (strstr(buffer, "BYE") != NULL)
        {
            break;
        }
    }
}

/**
 * @brief Select a folder on the IMAP server
 *
 * @param sockfd
 * @param buffer
 * @param folder
 */
void select_folder(int sockfd, char *buffer, const char *folder)
{
    char sanitized_folder[BUFFER_SIZE];
    strncpy(sanitized_folder, folder, BUFFER_SIZE - 1);
    sanitized_folder[BUFFER_SIZE - 1] = '\0';
    sanitize_input(sanitized_folder);

    snprintf(buffer, BUFFER_SIZE, "%s SELECT \"%s\"\r\n", generate_tag(), folder);

    if (send(sockfd, buffer, strlen(buffer), 0) < 0)
    {
        fprintf(stderr, "Failed to send SELECT command\n");
        exit(3);
    }

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t nbytes = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (nbytes <= 0)
        {
            imap_logout(sockfd, buffer);
            close(sockfd);
            exit(3);
        }

        if (strstr(buffer, "OK") != NULL)
        {
            break;
        }
        else if (strstr(buffer, "NO") != NULL)
        {
            printf("Folder not found\n");
            imap_logout(sockfd, buffer);
            close(sockfd);
            exit(3);
        }
        else if (strstr(buffer, "BAD") != NULL)
        {
            printf("Invalid command or arguments\n");
            imap_logout(sockfd, buffer);
            close(sockfd);
            exit(3);
        }
    }
}

/**
 * @brief Fetch a specific email by message number
 *
 * @param sockfd
 * @param buffer
 * @param messageNum
 */
void fetch_mail(int sockfd, char *buffer, const char *messageNum)
{
    // Validate messageNum for seq-number rule only
    if (!is_valid_seq_number(messageNum))
    {
        imap_logout(sockfd, buffer);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    snprintf(buffer, BUFFER_SIZE, "%s FETCH %s BODY.PEEK[]\r\n", generate_tag(), messageNum ? messageNum : "*");

    if (send(sockfd, buffer, strlen(buffer), 0) < 0)
    {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    size_t buffer_size = BUFFER_SIZE;
    char *response = malloc(buffer_size);
    if (!response)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    size_t total_read = 0;
    ssize_t nbytes;

    while ((nbytes = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0)
    {

        if (total_read + nbytes + 1 > buffer_size)
        {
            buffer_size *= 2; // Double the buffer size
            char *new_response = realloc(response, buffer_size);
            if (!new_response)
            {
                perror("realloc failed");
                free(response);
                exit(EXIT_FAILURE);
            }
            response = new_response;
        }

        memcpy(response + total_read, buffer, nbytes);
        total_read += nbytes;

        // Null-terminate the received data to prevent overflow in strstr
        response[total_read] = '\0';

        // Check for completion tag in the entire response, not just the last received part
        if (strstr(response, "OK Fetch completed"))
        {
            break;
        }

        if (strstr(response, "A03 BAD Error in IMAP command"))
        {
            printf("Message not found\n");
            free(response);
            imap_logout(sockfd, buffer);
            close(sockfd);
            exit(3);
        }
    }

    if (nbytes < 0)
    {
        perror("recv failed");
        free(response);
        imap_logout(sockfd, buffer);
        close(sockfd);
        exit(3);
    }
    else if (nbytes == 0)
    {
        printf("Connection closed by the server\n");
        free(response);
        imap_logout(sockfd, buffer);
        close(sockfd);
        exit(3);
    }

    // Handle response data
    extract_email_content(response);

    free(response); // Free the dynamically allocated memory
}

/**
 * @brief Parse email headers (FROM, TO, DATE, SUBJECT)
 *
 * @param sockfd
 * @param buffer
 * @param messageNum
 */
void parse_mail(int sockfd, char *buffer, const char *messageNum)
{
    char from[BUFFER_SIZE] = "From:";
    char to[BUFFER_SIZE] = "To:";
    char date[BUFFER_SIZE] = "Date:";
    char subject[BUFFER_SIZE] = "Subject: <No subject>";

    snprintf(buffer, BUFFER_SIZE,
             "%s FETCH %s BODY.PEEK[HEADER.FIELDS (FROM TO DATE SUBJECT)]\r\n",
             generate_tag(),
             messageNum);

    if (send(sockfd, buffer, strlen(buffer), 0) < 0)
        exit(EXIT_FAILURE);

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE * 10);
        ssize_t nbytes = recv(sockfd, buffer, BUFFER_SIZE * 10, 0);
        if (nbytes <= 0)
        {
            imap_logout(sockfd, buffer);
            close(sockfd);
            exit(3);
        }

        if (strstr(buffer, "OK Fetch completed") != NULL)
        {
            break;
        }
        else if (strstr(buffer, "NO") != NULL)
        {
            printf("Folder not found\n");
            imap_logout(sockfd, buffer);
            close(sockfd);
            exit(3);
        }
        else if (strstr(buffer, "BAD") != NULL)
        {
            printf("Invalid command or arguments\n");
            imap_logout(sockfd, buffer);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
    }

    char *line = strtok(buffer, "\r\n");

    while (line != NULL)
    {
        // Handle folded lines by unfolding them
        char *next_line = strtok(NULL, "\r\n");

        // Append continuation lines
        while (next_line && (next_line[0] == ' ' || next_line[0] == '\t'))
        {
            size_t len = strlen(line);
            if (line[len - 1] != ' ' && line[len - 1] != ':')
            {
                line[len] = ' '; // Add a space if not already spaced
                line[len + 1] = '\0';
            }
            strcat(line, next_line + 1);
            next_line = strtok(NULL, "\r\n");
        }

        // Process lines based on their headers
        if (strncasecmp(line, "From:", 5) == 0)
        {
            strcpy(from + 5, line + 5);
        }
        else if (strncasecmp(line, "To:", 3) == 0)
        {
            strcpy(to + 3, line + 3);
        }
        else if (strncasecmp(line, "Date:", 5) == 0)
        {
            strcpy(date + 5, line + 5);
        }
        else if (strncasecmp(line, "Subject:", 8) == 0)
        {
            strcpy(subject + 8, line + 8);
        }
        line = next_line;
    }

    printf("%s\n%s\n%s\n%s\n", from, to, date, subject);
}

/**
 * @brief List the subjects of all emails in the folder
 *
 * @param sockfd
 * @param buffer
 */
void subjects_list(int sockfd, char *buffer)
{
    // Send command to fetch all subjects from all messages
    snprintf(buffer, BUFFER_SIZE, "%s FETCH 1:* (BODY[HEADER.FIELDS (SUBJECT)])\r\n", generate_tag());
    if (send(sockfd, buffer, strlen(buffer), 0) < 0)
    {
        exit(EXIT_FAILURE);
    }

    int complete = 0;
    char *response = calloc(1, BUFFER_SIZE * 10); // Allocate memory to store the response
    if (!response)
    {
        perror("Failed to allocate memory for response");
        exit(EXIT_FAILURE);
    }

    while (!complete)
    {
        memset(buffer, 0, BUFFER_SIZE * 10);
        ssize_t nbytes = recv(sockfd, buffer, BUFFER_SIZE * 10, 0);
        if (nbytes <= 0)
        {
            imap_logout(sockfd, buffer);
            close(sockfd);
            exit(3);
        }

        strcat(response, buffer); // Append new data to response

        if (strstr(buffer, "OK Fetch completed") != NULL)
        {
            complete = 1;
        }
    }

    char *line = strtok(response, "\r\n");
    char subject[BUFFER_SIZE] = "Subject:";
    int count = 1;
    while (line != NULL)
    {
        char *next_line = strtok(NULL, "\r\n");

        while (next_line && (next_line[0] == ' ' || next_line[0] == '\t'))
        {
            size_t len = strlen(line);
            if (line[len - 1] != ' ' && line[len - 1] != ':')
            {
                line[len] = ' ';
                line[len + 1] = '\0';
            }
            strcat(line, next_line + 1);
            next_line = strtok(NULL, "\r\n");
        }

        if (strncasecmp(line, "Subject:", 8) == 0)
        {
            strcpy(subject + 8, line + 8);
            printf("%d:%s\n", count, subject + 8);
            count++;
        }

        if (strncasecmp(line, "*", 1) == 0 && strncasecmp(next_line, ")", 1) == 0)
        {
            printf("%d: <No subject>\n", count);
            count++;
        }

        line = next_line;
    }

    free(response);
    free(line);
}

/**
 * @brief Fetch a specific email in MIME format by message number
 * @param sockfd
 * @param buffer
 * @param messageNum

 */
void fetch_mail_mime(int sockfd, char *buffer, const char *messageNum)
{
    const char *tag = generate_tag();
    if (!messageNum)
    {
        snprintf(buffer, BUFFER_SIZE, "%s FETCH * BODY.PEEK[]\r\n", tag); // Fetch the last message
    }
    else
    {
        snprintf(buffer, BUFFER_SIZE, "%s FETCH %s BODY.PEEK[]\r\n", tag, messageNum);
    }

    if (send(sockfd, buffer, strlen(buffer), 0) < 0)
    {
        perror("send failed");
        exit(EXIT_FAILURE);
    }

    size_t buffer_size = BUFFER_SIZE;
    char *response = malloc(buffer_size);
    if (!response)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    size_t total_read = 0;
    ssize_t nbytes;

    while ((nbytes = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        if (total_read + nbytes > buffer_size)
        {
            buffer_size *= 2;
            char *new_response = realloc(response, buffer_size);
            if (!new_response)
            {
                perror("realloc failed");
                free(response);
                exit(EXIT_FAILURE);
            }
            response = new_response;
        }

        memcpy(response + total_read, buffer, nbytes);
        total_read += nbytes;

        response[total_read] = '\0';

        if (strstr(response, "OK Fetch completed"))
        {
            break;
        }

        if (strstr(response, "A03 BAD Error in IMAP command"))
        {
            printf("Message not found\n");
            exit(3);
        }
    }

    if (nbytes < 0)
    {
        perror("recv failed");
        imap_logout(sockfd, buffer); // logout before close
        close(sockfd);
        exit(3);
    }
    else if (nbytes == 0)
    {
        printf("Connection closed by the server\n");
        imap_logout(sockfd, buffer); // logout before close
        close(sockfd);
        exit(3);
    }

    // Handle response data
    decode_mime_message(response);

    free(response); // Free the dynamically allocated memory
}

// ########################## OPEN_SSL ######################################

/**
 * @brief Login to IMAP server with OpenSSL
 * 
 *
 * @param ssl 
 * @param buffer 
 * @param username 
 * @param password 
 */
void imap_login_ssl(SSL *ssl, char *buffer, const char *username, const char *password)
{
    snprintf(buffer, BUFFER_SIZE, "%s LOGIN %s %s\r\n", generate_tag(), username, password);

    if (SSL_write(ssl, buffer, strlen(buffer)) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(3);
    }

    char response[BUFFER_SIZE] = {0};
    size_t response_length = 0;
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t nbytes = SSL_read(ssl, buffer, BUFFER_SIZE);
        if (nbytes <= 0)
        {
            fprintf(stderr, "Server closed connection unexpectedly\n");
            SSL_free(ssl);
            exit(3);
        }
        strncat(response, buffer, nbytes);
        response_length += nbytes;

        if (strstr(buffer, "OK") != NULL)
        {
            if (strstr(response, "Logged in") != NULL)
            {
                break;
            }
        }
        else if (strstr(buffer, "NO") != NULL || strstr(buffer, "BAD") != NULL)
        {
            printf("Login failure\n");
            SSL_free(ssl);
            exit(3);
        }
    }
}

/**
 * @brief Logout of IMAP server with OpenSSl
 * 
 * @param ssl 
 * @param buffer 
 */
void imap_logout_ssl(SSL *ssl, char *buffer)
{
    snprintf(buffer, BUFFER_SIZE, "%s LOGOUT\r\n", generate_tag());

    if (SSL_write(ssl, buffer, strlen(buffer)) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t nbytes = SSL_read(ssl, buffer, BUFFER_SIZE);
        if (nbytes <= 0)
        {
            SSL_free(ssl);
            exit(3);
        }

        if (strstr(buffer, "BYE") != NULL)
        {
            break;
        }
    }
}

/**
 * @brief Selct Folder for server with OpenSSL
 * 
 *
 * @param ssl 
 * @param buffer 
 * @param folder 
 */
void select_folder_ssl(SSL *ssl, char *buffer, const char *folder)
{
    char sanitized_folder[BUFFER_SIZE];
    strncpy(sanitized_folder, folder, BUFFER_SIZE - 1);
    sanitized_folder[BUFFER_SIZE - 1] = '\0';
    sanitize_input(sanitized_folder);

    snprintf(buffer, BUFFER_SIZE, "%s SELECT \"%s\"\r\n", generate_tag(), folder);

    if (SSL_write(ssl, buffer, strlen(buffer)) <= 0)
    {
        fprintf(stderr, "Failed to send SELECT command\n");
        exit(3);
    }

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t nbytes = SSL_read(ssl, buffer, BUFFER_SIZE);
        if (nbytes <= 0)
        {
            imap_logout_ssl(ssl, buffer);
            SSL_free(ssl);
            exit(3);
        }

        if (strstr(buffer, "OK") != NULL)
        {
            break;
        }
        else if (strstr(buffer, "NO") != NULL || strstr(buffer, "BAD") != NULL)
        {
            printf("Folder not found or invalid command\n");
            imap_logout_ssl(ssl, buffer);
            SSL_free(ssl);
            exit(3);
        }
    }
}

/**
 * @brief Fetch email from server for SSL
 * @param ssl 
 * @param buffer 
 * @param messageNum 
 
 */
void fetch_mail_ssl(SSL *ssl, char *buffer, const char *messageNum)
{
    if (!is_valid_seq_number(messageNum))
    {
        imap_logout_ssl(ssl, buffer);
        SSL_free(ssl);
        exit(EXIT_FAILURE);
    }

    snprintf(buffer, BUFFER_SIZE, "%s FETCH %s BODY.PEEK[]\r\n", generate_tag(), messageNum ? messageNum : "*");

    if (SSL_write(ssl, buffer, strlen(buffer)) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    size_t buffer_size = BUFFER_SIZE;
    char *response = malloc(buffer_size);
    if (!response)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    size_t total_read = 0;
    ssize_t nbytes;

    while ((nbytes = SSL_read(ssl, buffer, BUFFER_SIZE)) > 0)
    {
        if (total_read + nbytes + 1 > buffer_size)
        {
            buffer_size *= 2;
            char *new_response = realloc(response, buffer_size);
            if (!new_response)
            {
                perror("realloc failed");
                free(response);
                exit(EXIT_FAILURE);
            }
            response = new_response;
        }

        memcpy(response + total_read, buffer, nbytes);
        total_read += nbytes;
        response[total_read] = '\0';

        if (strstr(response, "OK Fetch completed"))
        {
            break;
        }

        if (strstr(response, "A03 BAD Error in IMAP command"))
        {
            printf("Message not found\n");
            free(response);
            imap_logout_ssl(ssl, buffer);
            SSL_free(ssl);
            exit(3);
        }
    }

    if (nbytes < 0)
    {
        perror("recv failed");
        free(response);
        imap_logout_ssl(ssl, buffer);
        SSL_free(ssl);
        exit(3);
    }
    else if (nbytes == 0)
    {
        printf("Connection closed by the server\n");
        free(response);
        imap_logout_ssl(ssl, buffer);
        SSL_free(ssl);
        exit(3);
    }

    extract_email_content(response);
    free(response);
}

/**
 * @brief Parse email from server with SSL
 * 
 *
 * @param ssl 
 * @param buffer 
 * @param messageNum 
 */
void parse_mail_ssl(SSL *ssl, char *buffer, const char *messageNum)
{
    char from[BUFFER_SIZE] = "From:";
    char to[BUFFER_SIZE] = "To:";
    char date[BUFFER_SIZE] = "Date:";
    char subject[BUFFER_SIZE] = "Subject: <No subject>";

    snprintf(buffer, BUFFER_SIZE, "%s FETCH %s BODY.PEEK[HEADER.FIELDS (FROM TO DATE SUBJECT)]\r\n", generate_tag(), messageNum);

    if (SSL_write(ssl, buffer, strlen(buffer)) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE * 10);
        ssize_t nbytes = SSL_read(ssl, buffer, BUFFER_SIZE * 10);
        if (nbytes <= 0)
        {
            imap_logout_ssl(ssl, buffer);
            SSL_free(ssl);
            exit(3);
        }

        if (strstr(buffer, "OK Fetch completed") != NULL)
        {
            break;
        }
        else if (strstr(buffer, "NO") != NULL || strstr(buffer, "BAD") != NULL)
        {
            printf("Folder not found or invalid command\n");
            imap_logout_ssl(ssl, buffer);
            SSL_free(ssl);
            exit(EXIT_FAILURE);
        }
    }

    char *line = strtok(buffer, "\r\n");

    while (line != NULL)
    {
        char *next_line = strtok(NULL, "\r\n");

        while (next_line && (next_line[0] == ' ' || next_line[0] == '\t'))
        {
            size_t len = strlen(line);
            if (line[len - 1] != ' ' && line[len - 1] != ':')
            {
                line[len] = ' ';
                line[len + 1] = '\0';
            }
            strcat(line, next_line + 1);
            next_line = strtok(NULL, "\r\n");
        }

        if (strncasecmp(line, "From:", 5) == 0)
        {
            strcpy(from + 5, line + 5);
        }
        else if (strncasecmp(line, "To:", 3) == 0)
        {
            strcpy(to + 3, line + 3);
        }
        else if (strncasecmp(line, "Date:", 5) == 0)
        {
            strcpy(date + 5, line + 5);
        }
        else if (strncasecmp(line, "Subject:", 8) == 0)
        {
            strcpy(subject + 8, line + 8);
        }
        line = next_line;
    }

    printf("%s\n%s\n%s\n%s\n", from, to, date, subject);
}

/**
 * @brief List Subjects for SSl
 * 
 *
 * @param ssl 
 * @param buffer 
 */
void subjects_list_ssl(SSL *ssl, char *buffer)
{
    snprintf(buffer, BUFFER_SIZE, "%s FETCH 1:* (BODY[HEADER.FIELDS (SUBJECT)])\r\n", generate_tag());
    if (SSL_write(ssl, buffer, strlen(buffer)) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    int complete = 0;
    char *response = calloc(1, BUFFER_SIZE * 10);
    if (!response)
    {
        perror("Failed to allocate memory for response");
        exit(EXIT_FAILURE);
    }

    while (!complete)
    {
        memset(buffer, 0, BUFFER_SIZE * 10);
        ssize_t nbytes = SSL_read(ssl, buffer, BUFFER_SIZE * 10);
        if (nbytes <= 0)
        {
            imap_logout_ssl(ssl, buffer);
            SSL_free(ssl);
            exit(3);
        }

        strcat(response, buffer);

        if (strstr(buffer, "OK Fetch completed") != NULL)
        {
            complete = 1;
        }
    }

    char *line = strtok(response, "\r\n");
    char subject[BUFFER_SIZE] = "Subject:";
    int count = 1;
    while (line != NULL)
    {
        char *next_line = strtok(NULL, "\r\n");

        while (next_line && (next_line[0] == ' ' || next_line[0] == '\t'))
        {
            size_t len = strlen(line);
            if (line[len - 1] != ' ' && line[len - 1] != ':')
            {
                line[len] = ' ';
                line[len + 1] = '\0';
            }
            strcat(line, next_line + 1);
            next_line = strtok(NULL, "\r\n");
        }

        if (strncasecmp(line, "Subject:", 8) == 0)
        {
            strcpy(subject + 8, line + 8);
            printf("%d:%s\n", count, subject + 8);
            count++;
        }

        if (strncasecmp(line, "*", 1) == 0 && strncasecmp(next_line, ")", 1) == 0)
        {
            printf("%d: <No subject>\n", count);
            count++;
        }

        line = next_line;
    }

    free(response);
    free(line);
}

/**
 * @brief MIME for SSL
 * 
 *
 * @param ssl 
 * @param buffer 
 * @param messageNum 
 */
void fetch_mail_mime_ssl(SSL *ssl, char *buffer, const char *messageNum)
{
    if (!messageNum)
    {
        snprintf(buffer, BUFFER_SIZE, "%s FETCH * BODY.PEEK[]\r\n", generate_tag());
    }
    else
    {
        snprintf(buffer, BUFFER_SIZE, "%s FETCH %s BODY.PEEK[]\r\n", generate_tag(), messageNum);
    }

    if (SSL_write(ssl, buffer, strlen(buffer)) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    size_t buffer_size = BUFFER_SIZE;
    char *response = malloc(buffer_size);
    if (!response)
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    size_t total_read = 0;
    ssize_t nbytes;

    while ((nbytes = SSL_read(ssl, buffer, BUFFER_SIZE)) > 0)
    {
        if (total_read + nbytes > buffer_size)
        {
            buffer_size *= 2;
            char *new_response = realloc(response, buffer_size);
            if (!new_response)
            {
                perror("realloc failed");
                free(response);
                exit(EXIT_FAILURE);
            }
            response = new_response;
        }

        memcpy(response + total_read, buffer, nbytes);
        total_read += nbytes;
        response[total_read] = '\0';

        if (strstr(response, "OK Fetch completed"))
        {
            break;
        }

        if (strstr(response, "A03 BAD Error in IMAP command"))
        {
            printf("Message not found\n");
            exit(3);
        }
    }

    if (nbytes < 0)
    {
        perror("recv failed");
        imap_logout_ssl(ssl, buffer);
        SSL_free(ssl);
        exit(3);
    }
    else if (nbytes == 0)
    {
        printf("Connection closed by the server\n");
        imap_logout_ssl(ssl, buffer);
        SSL_free(ssl);
        exit(3);
    }

    decode_mime_message(response);
    free(response);
}

// ##########################################################################