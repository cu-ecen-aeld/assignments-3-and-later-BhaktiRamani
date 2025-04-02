/*******************************************************************************
* @file       aesdsocket.c
* @brief      A socket server implementation that handles client connections,
*             stores received data, and sends it back to clients
*
* This program creates a socket server that:
* - Listens on port 9000 for incoming connections
* - Accepts client connections and receives data
* - Stores received data in /dev/aesdchar
* - Sends accumulated data back to connected clients
* - Supports daemon mode with -d argument
* - Handles SIGTERM and SIGINT signals gracefully
*
* @author     Bhakti Ramani
* @date       Feb 23 2025
* @version    1.0
* @copyright  Copyright (c) Bhakti Ramani, All Rights Reserved
*******************************************************************************/

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/queue.h>
#include "../aesd-char-driver/aesd_ioctl.h"

#define USE_AESD_CHAR_DEVICE 1

#define SEEK_COMMAND "AESDCHAR_IOCSEEKTO:"

#define DEBUG 0 
#if DEBUG == 1
#define LOG perror 
#else
#define LOG(...) do {} while (0)
#endif

/* Global variables */
bool exit_flag = false;
int sockfd, client_fd; // Removed global file_fd

/* Constants */
#define PORT "9000"
#ifdef USE_AESD_CHAR_DEVICE
    #define SOCKET_FILE "/dev/aesdchar"
#else
    #define SOCKET_FILE "/var/tmp/aesdsocketdata"
#endif

#define CLIENT_BUFFER_LEN 1024

/* Thread-related structures */
typedef struct thread_arg {
    int client_fd;
    struct sockaddr_storage socket_addr;
} thread_arg;

typedef struct thread_node {
    pthread_t threadId;
    SLIST_ENTRY(thread_node) entry;
} thread_node;

SLIST_HEAD(ThreadList, thread_node) head = SLIST_HEAD_INITIALIZER(head);

pthread_mutex_t mutex_read_write = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_thread_LL = PTHREAD_MUTEX_INITIALIZER;

/* Function declarations */
int send_rcv_socket_data(int client_fd, int file_fd);
int return_socketdata_to_client(int client_fd, int file_fd);

void* thread_operation(void *args) {
    thread_arg* thread_args = (thread_arg*)args;
    int client_fd_t = thread_args->client_fd;

    // Open the device file within the thread
    int file_fd_t = open(SOCKET_FILE, O_RDWR | O_APPEND | O_CREAT, 0666);
    if (file_fd_t == -1) {
        syslog(LOG_ERR, "ERROR: Failed to open %s in thread: %s", SOCKET_FILE, strerror(errno));
        close(client_fd_t);
        free(thread_args);
        return NULL;
    }

    // Perform read/write operations
    int written_bytes;
    if ((written_bytes = send_rcv_socket_data(client_fd_t, file_fd_t)) > 0) {
        return_socketdata_to_client(client_fd_t, file_fd_t);
    }

    // Clean up
    close(file_fd_t); // Close the device file
    close(client_fd_t);
    free(thread_args);
    return NULL;
}

void thread_node_add(pthread_t thread_id) {
    thread_node* node = (thread_node*)malloc(sizeof(thread_node));
    if (node == NULL) {
        syslog(LOG_INFO, "Thread node creation failed");
        return;
    }
    node->threadId = thread_id;
    pthread_mutex_lock(&mutex_thread_LL);
    SLIST_INSERT_HEAD(&head, node, entry);
    pthread_mutex_unlock(&mutex_thread_LL);
}

void thread_join() {
    thread_node *current, *next;
    pthread_mutex_lock(&mutex_thread_LL);
    current = SLIST_FIRST(&head);
    while (current != NULL) {
        next = SLIST_NEXT(current, entry);
        if (pthread_join(current->threadId, NULL) == 0) {
            SLIST_REMOVE(&head, current, thread_node, entry);
            free(current);
        }
        current = next;
    }
    pthread_mutex_unlock(&mutex_thread_LL);
}

bool create_daemon() {
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Fork failed for Daemon");
        return false;
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    if (setsid() < 0) {
        syslog(LOG_ERR, "New Session Creation failed for Daemon");
        return false;
    }
    if (chdir("/") == -1) {
        syslog(LOG_ERR, "Changing working directory failed for Daemon");
        return false;
    }
    int dev_null_fd = open("/dev/null", O_RDWR);
    if (dev_null_fd == -1) {
        LOG("Failed to open /dev/null");
        return false;
    }
    if (dup2(dev_null_fd, STDIN_FILENO) == -1 || 
        dup2(dev_null_fd, STDOUT_FILENO) == -1 || 
        dup2(dev_null_fd, STDERR_FILENO) == -1) {
        LOG("Failed to redirect stdio");
        close(dev_null_fd);
        return false;
    }
    close(dev_null_fd);
    return true;
}

void clean() {
    close(sockfd);
    closelog();
    syslog(LOG_INFO, "Cleaning and Closing the aesdsocket");
    exit(0);
}

void signal_handler(int signo) {
    if (signo == SIGINT || signo == SIGTERM) {
        exit_flag = true;
        syslog(LOG_DEBUG, "Caught signal, exiting");
    }
}

void reg_signal_handler(void) {
    struct sigaction sighandle = {
        .sa_handler = signal_handler,
        .sa_flags = 0
    };
    sigemptyset(&sighandle.sa_mask);
    if (sigaction(SIGINT, &sighandle, NULL) == -1 || 
        sigaction(SIGTERM, &sighandle, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up signal handler: %s", strerror(errno));
    }
}

bool ioctl_just_performed = false;

int send_rcv_socket_data(int client_fd, int file_fd) {
    char *client_buffer = calloc(CLIENT_BUFFER_LEN, sizeof(char));
    if (client_buffer == NULL) {
        syslog(LOG_ERR, "ERROR: Client Buffer Allocation failed");
        return -1;
    }

    size_t total_received = 0;
    size_t current_size = CLIENT_BUFFER_LEN;

    while (1) {
        ssize_t received = recv(client_fd, client_buffer + total_received,
                                current_size - total_received - 1, 0);
        if (received <= 0) {
            if (received == 0) {
                syslog(LOG_INFO, "Connection closed by client");
            } else {
                syslog(LOG_ERR, "recv failed: %s", strerror(errno));
            }
            free(client_buffer);
            return -1;
        }

        total_received += received;
        client_buffer[total_received] = '\0';

        if (memchr(client_buffer, '\n', total_received) != NULL) {
            break;
        }

        if (total_received >= current_size - 1) {
            size_t new_size = current_size * 2;
            char *new_buffer = realloc(client_buffer, new_size);
            if (new_buffer == NULL) {
                syslog(LOG_ERR, "ERROR: Buffer reallocation failed");
                free(client_buffer);
                return -1;
            }
            client_buffer = new_buffer;
            current_size = new_size;
        }
    }

    pthread_mutex_lock(&mutex_read_write);
    ssize_t written = 0;
    if (strncmp(client_buffer, SEEK_COMMAND, strlen(SEEK_COMMAND)) == 0) {
        struct aesd_seekto seek_params;
        if (sscanf(client_buffer + strlen(SEEK_COMMAND), "%u,%u", 
                   &seek_params.write_cmd, &seek_params.write_cmd_offset) == 2) {
            syslog(LOG_INFO, "Seeking to write_cmd: %u, offset: %u", 
                   seek_params.write_cmd, seek_params.write_cmd_offset);
            if (ioctl(file_fd, AESDCHAR_IOCSEEKTO, &seek_params) == -1) {
                syslog(LOG_ERR, "ERROR: ioctl seek failed: %s", strerror(errno));
                free(client_buffer);
                pthread_mutex_unlock(&mutex_read_write);
                return -1;
            }
            written = 1;
            ioctl_just_performed = true;
        } else {
            syslog(LOG_ERR, "ERROR: Invalid seek command format");
            free(client_buffer);
            pthread_mutex_unlock(&mutex_read_write);
            return -1;
        }
    } else {
        written = write(file_fd, client_buffer, total_received);
        if (written == -1 || written != total_received) {
            //syslog(LOG_ERR, "ERROR: File write failed: % economy", strerror(errno));
            free(client_buffer);
            pthread_mutex_unlock(&mutex_read_write);
            return -1;
        }
        ioctl_just_performed = false;
    }

    free(client_buffer);
    pthread_mutex_unlock(&mutex_read_write);
    return written;
}

int return_socketdata_to_client(int client_fd, int file_fd) {
    char *send_buffer = malloc(CLIENT_BUFFER_LEN);
    if (send_buffer == NULL) {
        syslog(LOG_ERR, "ERROR: Client buffer Allocation failed");
        return -1;
    }

    pthread_mutex_lock(&mutex_read_write);
    #ifdef USE_AESD_CHAR_DEVICE
    if (!ioctl_just_performed) {
        close(file_fd);
        file_fd = open(SOCKET_FILE, O_RDWR | O_APPEND | O_CREAT, 0666);
        if (file_fd == -1) {
            syslog(LOG_ERR, "ERROR: Reopening device failed: %s", strerror(errno));
            pthread_mutex_unlock(&mutex_read_write);
            free(send_buffer);
            return -1;
        }
    }
    ioctl_just_performed = false;
    #else
    if (lseek(file_fd, 0, SEEK_SET) == -1) {
        syslog(LOG_ERR, "ERROR: lseek failed: %s", strerror(errno));
        pthread_mutex_unlock(&mutex_read_write);
        free(send_buffer);
        return -1;
    }
    #endif

    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, send_buffer, CLIENT_BUFFER_LEN)) > 0) {
        ssize_t sent = send(client_fd, send_buffer, bytes_read, 0);
        if (sent == -1) {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "ERROR: Send to client failed: %s", strerror(errno));
            free(send_buffer);
            pthread_mutex_unlock(&mutex_read_write);
            return -1;
        }
    }

    free(send_buffer);
    pthread_mutex_unlock(&mutex_read_write);
    return 0;
}

int main(int argc, char **argv) {
    bool daemon_mode = (argc >= 2 && strcmp(argv[1], "-d") == 0);

    openlog("aesdsocket", LOG_PID | LOG_CONS | LOG_PERROR, LOG_USER);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        syslog(LOG_ERR, "ERROR: Socket Generation Failed");
        clean();
    }
    syslog(LOG_INFO, "Socket Generation Successful with sockfd: %d", sockfd);

    struct addrinfo hints = {0}, *serverInfo = NULL;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &serverInfo) != 0) {
        syslog(LOG_ERR, "ERROR: getaddrinfo operation failed");
        freeaddrinfo(serverInfo);
        clean();
    }
    syslog(LOG_INFO, "getaddrinfo successful");

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    if (bind(sockfd, serverInfo->ai_addr, serverInfo->ai_addrlen) != 0) {
        syslog(LOG_ERR, "Bind Operation failed");
        freeaddrinfo(serverInfo);
        clean();
    }
    syslog(LOG_INFO, "Bind successful");

    if (daemon_mode && !create_daemon()) {
        syslog(LOG_ERR, "Daemon creation failed, hence exiting");
        freeaddrinfo(serverInfo);
        clean();
    }

    if (listen(sockfd, 20) == -1) {
        syslog(LOG_ERR, "ERROR: Can't Listen");
        freeaddrinfo(serverInfo);
        clean();
    }
    syslog(LOG_INFO, "Listening successfully");

    reg_signal_handler();
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);

    while (!exit_flag) {
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
        if (client_fd == -1) {
            syslog(LOG_ERR, "ERROR: client fd generation failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        if (client_addr.ss_family == AF_INET) {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)&client_addr;
            inet_ntop(AF_INET, &(addr_in->sin_addr), client_ip, sizeof(client_ip));
        } else if (client_addr.ss_family == AF_INET6) {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&client_addr;
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), client_ip, sizeof(client_ip));
        }

        thread_arg *t_args = malloc(sizeof(thread_arg));
        if (!t_args) {
            syslog(LOG_ERR, "Failed to allocate thread argument");
            close(client_fd);
            continue;
        }

        t_args->client_fd = client_fd;
        t_args->socket_addr = client_addr;

        pthread_t thread_id;
        int err = pthread_create(&thread_id, NULL, thread_operation, t_args);
        if (err != 0) {
            syslog(LOG_ERR, "Error creating thread: %s", strerror(err));
            free(t_args);
            close(client_fd);
            continue;
        }

        thread_node_add(thread_id);
        thread_join();
    }

    thread_join();
    freeaddrinfo(serverInfo);
    clean();
    return 0;
}

