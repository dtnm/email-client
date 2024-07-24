#include "sys_header.h"
#include "utils.h"
#include "email_client.h"
#include "utils_ssl.h"


static int tag_counter = 0;

/**
 * @brief Generate a unique IMAP tag for each command
 * 
 * @return const char* 
 */
const char *generate_tag() {
    static char tag_buffer[16];
    sprintf(tag_buffer, "A%02d", ++tag_counter);
    return tag_buffer;
}

/**
 * @brief Sanitize user input to prevent injection attacks
 * 
 * @param input 
 */
void sanitize_input(char *input) {
    char *src = input, *dst = input;
    while (*src) {
        if (isalnum((unsigned char)*src) || *src == '_' || *src == '-' || *src == '.' || *src == '@') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

/**
 * @brief Set a timeout for socket operations
 * 
 * @param sockfd 
 * @param seconds 
 */
void set_socket_timeout(int sockfd, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Extract the content of an email from the server's response
 * @param response 
 
 */
void extract_email_content(const char *response)
{
    // Begin looking for the email content right after the response size line.
    const char *start = strstr(response, "\r\n");
    if (start)
    {
        start += 2; // Skip over the newline characters to the actual start of the email content.

        const char *end = strstr(start, "A03 OK Fetch completed");
        if (end)
        {
            // Print only the part of the response that contains the email headers and body.
            char *content = strndup(start, end - start - 4);
            if (content)
            {
                printf("%s", content);
                printf("\n");
                free(content);
            }
        }
        else
        {
            printf("End marker of the email content not found.\n");
        }
    }
    else
    {
        printf("Start of the email content not found.\n");
    }
}

/**
 * @brief Extract the boundary string from a MIME header
 * 
 * @param header 
 * @return char* 
 */
char *extract_boundary(const char *header)
{
    const char *key = "boundary=";
    const char *start = strstr(header, key);
    if (!start)
        return NULL;

    start += strlen(key);
    if (*start == '"')
        start++; // Skip the initial quote if it exists

    char *boundary = malloc(256);
    char *ptr = boundary;
    while (*start && *start != '"' && *start != '\r' && *start != '\n' && *start != ';')
    {
        *ptr++ = *start++;
    }
    *ptr = '\0';
    return boundary;
}

/**
 * @brief Decode a MIME message and print its content
 * 
 * @param response 
 */
void decode_mime_message(const char *response)
{
    const char *boundaryTag = "boundary=";
    char boundary[256] = "--";
    const char *contentTypeTag = "Content-Type: text/plain;";

    // Locate the boundary in the header
    const char *boundaryStart = strstr(response, boundaryTag);

    if (boundaryStart == NULL)
    {
        printf("Boundary not found.\n");
        exit(4);
    }
    boundaryStart += strlen(boundaryTag);
    const char *boundaryEnd = strchr(boundaryStart, '\n');

    // Construct the boundary delimiter
    strncat(boundary, boundaryStart, boundaryEnd - boundaryStart);

    extract_inside_quotes(boundary);

    // Begin parsing through the message parts
    char *partStart = strstr(response, boundary); // Start at the first boundary
    if (partStart == NULL)
    {
        printf("No MIME parts found.\n");
        exit(4);
    }
    partStart += strlen(boundary);

    while (partStart)
    {
        char *nextBoundary = strstr(partStart, boundary);
        if (!nextBoundary)
            break;
        // Check if this part has the correct content type
        char *contentTypeStart = strstr(partStart, contentTypeTag);
        if (contentTypeStart && contentTypeStart < nextBoundary)
        {
            char *contentStart = strstr(contentTypeStart, "\r\n\r\n") + 4; // Skip to the content
            if (contentStart && contentStart < nextBoundary)
            {
                char *contentEnd = nextBoundary;
                printf("%.*s", (int)(contentEnd - contentStart - 2), contentStart);
                break; // Since we only need the first matching part
            }
        }

        partStart = nextBoundary + strlen(boundary);
    }
}

/**
 * @brief Extract a string inside quotes
 * 
 * @param str 
 */
void extract_inside_quotes(char *str)
{
    char *start_quote = strchr(str, '"');
    if (start_quote != NULL)
    {
        char *end_quote = strchr(start_quote + 1, '"');
        if (end_quote != NULL)
        {
            // Calculate length of substring inside quotes
            size_t length = end_quote - start_quote - 1;
            // Move characters to overwrite the part inside quotes
            memmove(start_quote, start_quote + 1, length);
            // Null-terminate the string at the end of the extracted substring
            str[start_quote - str + length] = '\0';
        }
    }
}

/**
 * @brief Validate if a message number is a valid sequence number
 * 
 * @param messageNum 
 * @return int 
 */
int is_valid_seq_number(const char *messageNum)
{
    if (!messageNum || strlen(messageNum) == 0)
    {
        return 0; // Invalid if empty
    }

    if (strcmp(messageNum, "*") == 0)
    {
        return 1; // Valid if "*"
    }

    for (size_t i = 0; i < strlen(messageNum); i++)
    {
        if (!isdigit(messageNum[i]))
        {
            return 0; // Invalid if not a digit
        }
    }

    return 1;
}
