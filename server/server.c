#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

volatile sig_atomic_t keep_running = 1;

void handle_signal(int signal) {
    syslog(LOG_INFO, "Caught signal %d, exiting", signal);
    keep_running = 0;
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up SIGINT handler: %m");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up SIGTERM handler: %m");
        exit(EXIT_FAILURE);
    }
}

void daemonize() {
    pid_t pid;

    // Fork the process
    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Exit the parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Create a new session and set the child as session leader
    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    // Fork again to ensure the daemon is not a session leader
    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Exit the first child
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Set the file mode creation mask to 0
    umask(0);

    // Change the working directory to the root directory
    if (chdir("/") < 0) {
        perror("chdir");
        exit(EXIT_FAILURE);
    }

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int open_socket() {
    int sockfd;
    struct sockaddr_in server_addr;

    // Open connection to syslog
    openlog("socket_server", LOG_PID|LOG_CONS, LOG_DAEMON);

    // Create a stream socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        syslog(LOG_ERR, "Failed to create socket: %m");
        closelog();
        return -1;
    }

    // Set up the server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9000);

    // Bind the socket to the address and port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Failed to bind socket: %m");
        close(sockfd);
        closelog();
        return -1;
    }

    // Start listening for incoming connections
    if (listen(sockfd, 5) < 0) {
        syslog(LOG_ERR, "Failed to listen on socket: %m");
        close(sockfd);
        closelog();  // Close syslog before returning
        return -1;
    }

    syslog(LOG_INFO, "Socket opened and listening on port 9000.");

    // Close connection to syslog
    closelog();
 
    return sockfd;
}

int accept_connection(int listen_sockfd, char * client_ip, size_t ip_len) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Accept a connection from a client
    int client_sockfd = accept(listen_sockfd, (struct sockaddr *)&client_addr, &client_len);
    if (client_sockfd < 0) {
        syslog(LOG_ERR, "Failed to accept connection: %m");
        return -1;
    }

    // Convert the client's IP address to a string
    if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, ip_len) == NULL) {
        syslog(LOG_ERR, "Failed to convert client IP address: %m");
        close(client_sockfd);
        return -1;
    }

    // Log the accepted connection
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);

    return client_sockfd;
}

#define BUFFER_SIZE 1024

int receive_and_respond(int client_sockfd) {
    char *line_buffer = NULL;
    size_t line_buffer_size = 0;
    ssize_t bytes_received;
    int file_fd;
    size_t buffer_capacity = BUFFER_SIZE;

    line_buffer = malloc(buffer_capacity);
    if (!line_buffer) {
        syslog(LOG_ERR, "Failed to allocate memory for line buffer");
        return -1;
    }

    // Open the file for appending, create it if it doesn't exist
    file_fd = open("/var/tmp/aesdsocketdata", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (file_fd < 0) {
        syslog(LOG_ERR, "Failed to open or create file: %m");
        return -1;
    }

    while ((bytes_received = recv(client_sockfd,
                    line_buffer + line_buffer_size, 
                    buffer_capacity - line_buffer_size - 1, 0)) > 0) {
        line_buffer_size += bytes_received;
        line_buffer[line_buffer_size] = '\0';  // Null-terminate the buffer

        // Check if the received data contains a newline character
        char *newline_pos = strchr(line_buffer, '\n');
        while (newline_pos != NULL) {
            size_t line_length = newline_pos - line_buffer + 1;

            // Write the complete line to the file
            if (write(file_fd, line_buffer, line_length) != line_length) {
                syslog(LOG_ERR, "Failed to write to file: %m");
                free(line_buffer);
                close(file_fd);
                return -1;
            }

            // Send the full content of the file back to the client
            close(file_fd); // Close and reopen the fle for reading
            file_fd = open("/var/tmp/aesdsocketdata", O_RDONLY);
            if (file_fd < 0) {
                syslog(LOG_ERR, "Failed to open fle for reading: %m");
                free(line_buffer);
                return -1;
            }

            char send_buffer[BUFFER_SIZE];
            ssize_t bytes_send;
            while ((bytes_send = read(file_fd, send_buffer, BUFFER_SIZE)) > 0) {
                if (send(client_sockfd, send_buffer, bytes_send, 0) != bytes_send) {
                    syslog(LOG_ERR, "Failed to send data to client: %m");
                    close(file_fd);
                    free(line_buffer);
                    return -1;
                }
            }
            if (bytes_send < 0) {
                syslog(LOG_ERR, "Failed to send the file: %m");
                close(file_fd);
                free(line_buffer);
                return -1;
            }
            // Close the fle descriptor before returning
            close(file_fd);

            // Move any remaining part of the buffer to the start for the next read
            size_t remaining = line_buffer_size - line_length;
            if (remaining > 0){
                memmove(line_buffer, line_buffer + line_length, remaining);
                line_buffer_size = remaining;
                newline_pos = strchr(line_buffer, '\n');
            }
            else {
                break;
            }
        }

        /* Reduce buffer size to fit remaining data
        if (buffer_capacity > BUFFER_SIZE) {
            line_buffer = realloc(line_buffer, BUFFER_SIZE);
            if (!line_buffer) {
                syslog(LOG_ERR, "Failed to reallocate memory for line buffer");
                close(file_fd);
                return -1;
            }
        }*/

        // check if buffer needs expansion
        if (line_buffer_size >= buffer_capacity - 1) {
            buffer_capacity += BUFFER_SIZE;
            // If buffer is full and no newline is found, expand it
            line_buffer = realloc(line_buffer, buffer_capacity);
            if (!line_buffer) {
                syslog(LOG_ERR, "Failed to reallocate memory for line buffer");
                close(file_fd);
                return -1;
            }
        }
    }

    if (bytes_received < 0) {
        syslog(LOG_ERR, "Failed to receive data: %m");
        free(line_buffer);
        close(file_fd);
        return -1;
    }

    // Clean up
    free(line_buffer);
    close(file_fd);
    return 0;
}


int main(int argc, char *argv[]) {
    int daemon_mode = 0;

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }
    // Open connection to syslog
    openlog("socket_server", LOG_PID|LOG_CONS, LOG_DAEMON);

    setup_signal_handler();

    remove("/var/tmp/aesdsocketdata");
    int listen_sockfd = open_socket(); // Assume open_socket() is defined as before
    if (listen_sockfd == -1) {
        syslog(LOG_ERR, "Error opening listening socket.");
        closelog();
        return EXIT_FAILURE;
    }
    if (daemon_mode) {
        daemonize();
    }
    while (keep_running) {
        char client_ip[INET_ADDRSTRLEN];
        int client_sockfd = accept_connection(listen_sockfd, client_ip, sizeof(client_ip));
        if (client_sockfd == -1) {
            if (errno == EINTR) {
                // Interrupted by signal, safe to continue
                continue;
            }
            syslog(LOG_ERR, "Error accepting a connection.");
            break;
        }

        // Handle the connection with client_sockfd
        if (receive_and_respond(client_sockfd) == -1) {
            syslog(LOG_ERR, "Error receiving data from client.");
        }

        syslog(LOG_INFO, "Closed connection from %s", client_ip);

        // Close the client socket
        close(client_sockfd);
    }

    // Close the listening socket
    close(listen_sockfd);

    remove("/var/tmp/aesdsocketdata");

    // Close connection to syslog
    closelog();

    return EXIT_SUCCESS;
}

