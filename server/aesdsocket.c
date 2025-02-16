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

#define PORT "9000"
#define SOCKET_FILE "/var/tmp/aesdsocketdata"
#define CLIENT_BUFFER_LEN 1024

bool exit_flag = false;
int sockfd, client_fd, file_fd;

bool create_daemon()
{
    pid_t pid;
    pid = fork();
    bool status = false;
    int dev_null_fd;

    if (pid < 0)
    {
        syslog(LOG_ERR, "Fork failed");
        return status;
    }

    if (pid > 0)
    {
        // Parent process hence exit
        exit(EXIT_SUCCESS);
    }

    // create new group and session
    if (setsid() < 0)
    {
        syslog(LOG_ERR, "Create new session  failed");
        return status;
    }

    // Change the working directory to "/"
    if (chdir("/") == -1)
    {
        syslog(LOG_ERR, "Changing working directory failed");
        return status;
    }
    // Since no files were open in parent, no fds are closed here
    //  Redirect STDIN , STDOUT and STDERR to /dev/null
    dev_null_fd = open("/dev/null", O_RDWR);
    if (dev_null_fd == -1)
    {
        perror("Failed to open /dev/null");
        return status;
    }

    // Redirect stdin (fd 0) to /dev/null
    if (dup2(dev_null_fd, STDIN_FILENO) == -1)
    {
        perror("Failed to redirect stdin");
        close(dev_null_fd);
        return status;
    }

    // Redirect stdout (fd 1) to /dev/null
    if (dup2(dev_null_fd, STDOUT_FILENO) == -1)
    {
        perror("Failed to redirect stdout");
        close(dev_null_fd);
        return status;
    }

    // Redirect stderr (fd 2) to /dev/null
    if (dup2(dev_null_fd, STDERR_FILENO) == -1)
    {
        perror("Failed to redirect stderr");
        close(dev_null_fd);
        return status;
    }

    // Close the original /dev/null file descriptor
    close(dev_null_fd);
    return true;
}

void clean()
{
    close(sockfd);
    close(client_fd);
    close(file_fd);
    closelog();
    syslog(LOG_INFO, "Cleaning and Closing");
    exit(1);
}
void signal_handler(int signo)
{
    if ((signo == SIGINT) || (signo == SIGTERM))
    {
        exit_flag = true;
        syslog(LOG_DEBUG, "Caught signal, exiting");
        // some clean up
    }
}

void reg_signal_handler(void)
{
        struct sigaction sighandle;
    //Initialize sigaction
    sighandle.sa_handler = signal_handler;
    sigemptyset(&sighandle.sa_mask);  // Initialize the signal set to empty
    sighandle.sa_flags = 0;            // No special flags

    // Catch SIGINT
    if (sigaction(SIGINT, &sighandle, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up signal handler SIGINT: %s \n", strerror(errno));
    }

    // Catch SIGTERM
    if (sigaction(SIGTERM, &sighandle, NULL) == -1) {
        syslog(LOG_ERR, "Error setting up signal handler SIGINT: %s \n", strerror(errno));
     }
}

int send_rcv_socket_data(int client_fd, int file_fd)
{
    char *client_buffer = NULL;
    size_t total_received = 0;
    size_t current_size = CLIENT_BUFFER_LEN;

    // Allocate initial buffer
    client_buffer = (char *)calloc(current_size, sizeof(char));
    if (client_buffer == NULL)
    {
        perror("[-] calloc");
        syslog(LOG_ERR, "ERROR: Client Buffer Allocation failed");
        return -1;
    }

    // Receive data until newline is found
    while (1)
    {
        ssize_t received = recv(client_fd, client_buffer + total_received,
                                current_size - total_received - 1, 0);

        if (received <= 0)
        {
            if (received == 0)
            {
                syslog(LOG_INFO, "Connection closed by client");
            }
            else
            {
                perror("[-] recv");
                syslog(LOG_ERR, "recv failed: %s", strerror(errno));
            }
            free(client_buffer);
            return -1;
        }

        total_received += received;
        client_buffer[total_received] = '\0';

        // Check for newline
        if (memchr(client_buffer, '\n', total_received) != NULL)
        {
            break;
        }

        // Resize buffer if needed
        if (total_received >= current_size - 1)
        {
            size_t new_size = current_size * 2;
            char *new_buffer = realloc(client_buffer, new_size);
            if (new_buffer == NULL)
            {
                perror("[-] realloc");
                syslog(LOG_ERR, "ERROR: Buffer reallocation failed");
                free(client_buffer);
                return -1;
            }
            client_buffer = new_buffer;
            current_size = new_size;
        }
    }

    // Reset file position to beginning before writing
    if (lseek(file_fd, 0, SEEK_SET) == -1)
    {
        perror("[-] lseek");
        syslog(LOG_ERR, "ERROR: lseek failed: %s", strerror(errno));
        free(client_buffer);
        return -1;
    }

    //Truncate the file to ensure clean write
    // if (ftruncate(file_fd, 0) == -1)
    // {
    //     perror("[-] ftruncate");
    //     syslog(LOG_ERR, "ERROR: ftruncate failed: %s", strerror(errno));
    //     free(client_buffer);
    //     return -1;
    // }

    // Write data to file
    ssize_t written = write(file_fd, client_buffer, total_received);
    if (written == -1 || written != total_received)
    {
        perror("[-] write");
        syslog(LOG_ERR, "ERROR: File write failed: %s", strerror(errno));
        free(client_buffer);
        return -1;
    }
    printf("[+] Written %ld bytes \n", total_received);
    // Ensure data is written to disk
    if (fdatasync(file_fd) == -1)
    {
        perror("[-] fdatasync");
        syslog(LOG_ERR, "ERROR: fdatasync failed: %s", strerror(errno));
        free(client_buffer);
        return -1;
    }

    free(client_buffer);
    return 0;
}

int return_socketdata_to_client(int client_fd, int file_fd)
{
    char *send_buffer;
    ssize_t bytes_read;

    // Reset file position to start
    lseek(file_fd, 0, SEEK_SET);

    send_buffer = (char *)malloc(CLIENT_BUFFER_LEN);
    if (send_buffer == NULL)
    {
        perror("[-] malloc");
        syslog(LOG_ERR, "ERROR : Client buffer Allocation failed");
        return -1;
    }

    // Read and send data
    while ((bytes_read = read(file_fd, send_buffer, CLIENT_BUFFER_LEN - 1)) > 0)
    {
        // ssize_t bytes_sent = 0;
        // while (bytes_sent < bytes_read)
        // {
        ssize_t sent = send(client_fd, send_buffer, bytes_read, 0);
        if (sent == -1)
        {
            if (errno == EINTR)
                continue;
            perror("[-] send");
            syslog(LOG_ERR, "ERROR: Send to client failed: %s",
                    strerror(errno));
            free(send_buffer);
            return -1;
        }
        printf("[+] Send %ld bytes \n", bytes_read);
        // bytes_sent += sent;
        // }
    }

    free(send_buffer);
    return 0;
}

int main(int argc, char **argv)
{

    const char *dir_path = "/var/tmp/";
    const char *file_name = "aesdsocketdata";

    // Construct full file path
    char full_path[512]; // Make sure buffer is large enough
    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, file_name);

    // Attempt to delete the file
    if (remove(full_path) == 0) {
        printf("File '%s' deleted successfully.\n", full_path);
    } else {
        perror("Error deleting file");
    }

    bool daemon_mode = false;

    // Check if the application to be run in daemon mode
    if ((argc >= 2) && (strcmp(argv[1], "-d") == 0))
    {
        daemon_mode = true;
    }

    // Open log - need change
    openlog("aesdsocket", LOG_PID | LOG_CONS | LOG_PERROR, LOG_USER);



    // Create a Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("[-] Socket");
        syslog(LOG_ERR, "ERROR : Socket Generation Failed");
        // perror("Socket Generation Failed \n");
        clean();
    }
    syslog(LOG_INFO, "Socket Generation Successfull with sockfd : %d", sockfd);
    printf("[+] Listening on port 9000 \n");

    // Bind - Assigning address to the socket
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo *serverInfo = NULL;

    if (getaddrinfo(NULL, PORT, &hints, &serverInfo) != 0)
    {
        syslog(LOG_ERR, "ERROR : getaddrinfo operation fail, can't get socket address");
   
        freeaddrinfo(serverInfo);
       
     
        clean();
    }
    printf("[+] Getaddr successfull \n");

    syslog(LOG_INFO, "getaddrinfo successfull");

    
    printf("[+] Getaddr successfull \n");

    int yes = 1;
    socklen_t size = sizeof(yes);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, size);
    // Bind
    if (bind(sockfd, serverInfo->ai_addr, serverInfo->ai_addrlen) != 0)
    {
        perror("[-] bind \n");
        syslog(LOG_ERR, "Bind Operation failed");

        // clearing the serverInfo
        if (serverInfo != NULL)
        {
            freeaddrinfo(serverInfo);
        }

        clean();
    }

    syslog(LOG_INFO, "bind successfull");
    printf("[+] Bind successfull \n");

    // Check if daemon to be created
    if (daemon_mode)
    {
        if (!create_daemon())
        {
            syslog(LOG_ERR, "Daemon creation failed, hence exiting");
            freeaddrinfo(serverInfo); // free the linked-list
            clean();
        }
        printf("[+] Deamon created \n");
    }

    if (listen(sockfd, 20) == -1)
    {
        syslog(LOG_ERR, "ERROR : Can't Listen");

        clean();
    }
    printf("[+] Listening on port 9000 \n");
    syslog(LOG_INFO, "Listeing successfully");


    file_fd = open(SOCKET_FILE, O_RDWR | O_APPEND | O_CREAT, 0666);
    if (file_fd == -1)
    {
        perror("[-] open");
        syslog(LOG_ERR, "ERROR : Generation of /var/tmp/aesdsocketdata failed");
        freeaddrinfo(serverInfo);
        clean();
    }

        // register signal handlers
    reg_signal_handler();
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
      

    while (!exit_flag)
    {
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
        if ((client_fd) == -1)
        {
            perror("[-] accpet");
            syslog(LOG_ERR, "ERROR : client fd generation failed");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        // Converting binary IP address to human readable format
        if (client_addr.ss_family == AF_INET)
        { // Check if the address is IPv4
            struct sockaddr_in *addr_in = (struct sockaddr_in *)&client_addr;
            inet_ntop(AF_INET, &(addr_in->sin_addr), client_ip, sizeof(client_ip));
        }
        else if (client_addr.ss_family == AF_INET6)
        { // Check if the address is IPv6
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&client_addr;
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), client_ip, sizeof(client_ip));
        }

        // Logging in the client ip
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        printf("[+] accepted client ip: %s\n", client_ip);




        // Receive packets from the client and store in SOCKETDATA_FILE
        if (send_rcv_socket_data(client_fd, file_fd) == 0)
        {
            // Send back the stored data of file back to the client
            return_socketdata_to_client(client_fd, file_fd);
        }

        close(client_fd);
     
        
    }
    freeaddrinfo(serverInfo);
}