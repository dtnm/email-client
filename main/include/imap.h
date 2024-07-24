#ifndef IMAP_H
#define IMAP_H

void imap_login(int sockfd, char *buffer, const char *username, const char *password);

void select_folder(int sockfd, char *buffer, const char *folder);

void fetch_mail(int sockfd, char *buffer, const char *messageNum);

void parse_mail(int sockfd, char *buffer, const char *messageNum);

void subjects_list(int sockfd, char *buffer);

void imap_logout(int sockfd, char *buffer);

void fetch_mail_mime(int sockfd, char *buffer, const char *messageNum);

// ############################ OPEN_SSL ######################################

void imap_login_ssl(SSL *ssl, char *buffer, const char *username, const char *password);

void select_folder_ssl(SSL *ssl, char *buffer, const char *folder);

void fetch_mail_ssl(SSL *ssl, char *buffer, const char *messageNum);

void parse_mail_ssl(SSL *ssl, char *buffer, const char *messageNum);

void subjects_list_ssl(SSL *ssl, char *buffer);

void imap_logout_ssl(SSL *ssl, char *buffer);

void fetch_mail_mime_ssl(SSL *ssl, char *buffer, const char *messageNum);

// ############################ OPEN_SSL #######################################


#endif
