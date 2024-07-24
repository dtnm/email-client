#include "email_client.h"
#include "utils.h"
#include "sys_header.h"
#include "utils_ssl.h"


/**
 * @brief Initialize the email client configuration with default values
 * 
 * @param config 
 */
void init_config(email_client_t *config) {
    config->username = NULL;
    config->password = NULL;
    config->folder = "INBOX";
    config->message_num = NULL;
    config->command = NULL;
    config->server_name = NULL;
    config->tls = 0;
}

/**
 * @brief Parse command-line arguments to populate the email client configuration
 * 
 * @param argc 
 * @param argv 
 * @param config 
 * @return int 
 */
int parse_command_line(int argc, char *argv[], email_client_t *config)
{
    int opt;

    // Parse command line arguments using getopt
    while ((opt = getopt(argc, argv, "u:p:f:n:t")) != -1)
    {
        switch (opt)
        {
        case 'u':
            config->username = optarg;
            break;
        case 'p':
            config->password = optarg;
            break;
        case 'f':
            config->folder = optarg;
            break;
        case 'n':
            config->message_num = optarg;
            break;
        case 't':
            config->tls = 1;
            break;
        case '?': // Unrecognized option or missing argument
            fprintf(stderr, "Usage: %s -u <username> -p <password> [-f <folder>] [-n <messageNum>] [-t] <command> <server_name>\n", argv[0]);
            return -1;
        }
    }

    // Check for the two non-option arguments: command and server_name
    if (optind < argc)
    {
        config->command = argv[optind++];
        sanitize_input(config->command);
    }
    if (optind < argc)
    {
        config->server_name = argv[optind++];
    }

    // Validate required fields
    if (!config->username || !config->password || !config->command || !config->server_name)
    {
        fprintf(stderr, "Missing required arguments.\n");
        exit(EXIT_FAILURE);
    }

    return 0; // Successfully parsed from command line
}
