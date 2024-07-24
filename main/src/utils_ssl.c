#include "utils_ssl.h"
#include "sys_header.h"

/**
 * @brief Initialise OpenSSL
 *
 */
void init_openssl()
{
    // SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

/**
 * @brief create SSL context for tasks
 *
 *
 * @return
 */
SSL_CTX *create_ssl_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

/**
 * @brief Clean up after use
 *
 */
void cleanup_openssl()
{
    EVP_cleanup();
}