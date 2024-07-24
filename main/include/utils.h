#ifndef UTILS_H
#define UTILS_H

void sanitize_input(char *input);

const char *generate_tag();

void set_socket_timeout(int sockfd, int seconds);

void extract_email_content(const char *response);

char *extract_boundary(const char *header);

void decode_mime_message(const char *response);

void extract_inside_quotes(char *str);

int is_valid_seq_number(const char *messageNum);

#endif
