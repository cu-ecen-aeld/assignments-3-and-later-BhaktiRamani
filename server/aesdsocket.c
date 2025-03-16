/*******************************************************************************
* @file       aesdsocket.c
* @brief      A socket server implementation that handles client connections,
*             stores received data, and sends it back to clients
*
* This program creates a socket server that:
* - Listens on port 9000 for incoming connections
* - Accepts client connections and receives data
* - Stores received data in /var/tmp/aesdsocketdata
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
#include <stdlib.h>
#include <pthread.h>
#include <sys/queue.h>
#include <time.h>

#define USE_AESD_CHAR_DEVICE 1



// for personal debug statements
// For personal debug statements
#define DEBUG 0 
#if DEBUG == 1
#define LOG perror 
#else
#define LOG(...) do {} while (0)
#endif



/* Global variables */
bool exit_flag = false;
int sockfd, client_fd, file_fd;

/* Constants */
#define PORT "9000"
#ifdef USE_AESD_CHAR_DEVICE
    #define SOCKET_FILE "/dev/aesdchar"
#else
    #define SOCKET_FILE "/var/tmp/aesdsocketdata"
#endif

//#ifndef USE_AESD_CHAR_DEVICE
//static pthread_t g_timer_thread;
//#endif


#define CLIENT_BUFFER_LEN 1024
FILE *tmp_file = NULL;

int send_rcv_socket_data(int client_fd, int file_fd);
int return_socketdata_to_client(int client_fd, int file_fd);

typedef struct thread_arg
{
    int client_fd;
    int file_fd;
    struct sockaddr_storage socket_addr;
}thread_arg;

typedef struct thread_node{
    pthread_t threadId;
    SLIST_ENTRY(thread_node ) entry;
}thread_node;

SLIST_HEAD(ThreadList, thread_node) head = SLIST_HEAD_INITIALIZER(head);

pthread_mutex_t mutex_read_write = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t mutex_thread_LL = PTHREAD_MUTEX_INITIALIZER;

void* thread_operation(void *args)
{
    thread_arg* thread_args = (thread_arg*)args;
    int client_fd_t = thread_args->client_fd;
    int file_fd_t = thread_args->file_fd;
    // read write operation in files
    int written_bytes;
    // Receive packets from the client and store in SOCKETDATA_FILE
    if ((written_bytes = send_rcv_socket_data(client_fd_t, file_fd_t)) > 0)
    {
        // Send back the stored data of file back to the client
        return_socketdata_to_client(client_fd_t, file_fd_t);
        
    }

    // Clean up
    close(client_fd_t);
    free(thread_args);
    return NULL;
}

void thread_node_add(pthread_t thread_id)
{
    thread_node* node = (thread_node*)malloc(sizeof(thread_node));
    if(node == NULL)
    {
        //printf("[-] Thread Node creation Failed \n");
        syslog(LOG_INFO, "Thread node creation failed");
        // some clean up        
    }
    node -> threadId = thread_id;
    pthread_mutex_lock(&mutex_thread_LL);
    syslog(LOG_INFO,"Inserting thread node");
    SLIST_INSERT_HEAD(&head,node,entry);
    pthread_mutex_unlock(&mutex_thread_LL);
}

void thread_join()
{
    thread_node* current = SLIST_FIRST(&head);
    thread_node *next = NULL; 

    pthread_mutex_lock(&mutex_thread_LL);
    while(current != NULL)
    {
  
        next = SLIST_NEXT(current,entry);
        
        if(pthread_join(current -> threadId, NULL) == 0)
        {
            //printf("[+] joining the thread and removing it from LL\n");
            SLIST_REMOVE(&head,current,thread_node,entry);
            free(current);
         
        }
        else{
            //printf("[-] Thread join failed \n");
        }
        current = next;

    }
    pthread_mutex_unlock(&mutex_thread_LL);
}

#ifndef USE_AESD_CHAR_DEVICE
void *timer_thread(void *args)
{
    thread_arg *arg = (thread_arg*)args;
    time_t t;
    struct tm *tmp;
    char outstr[200];
    while(!exit_flag)
    {
        sleep(10);
        t = time(NULL);
        tmp = localtime(&t);
        if (tmp == NULL) {
            perror("localtime");
            exit(EXIT_FAILURE);
        }

        if(strftime(outstr, sizeof(outstr), "timestamp:%Y-%m-%d %H:%M:%S\n", tmp) == 0)
        {
            //printf("[-] Timer failed \n");
            exit(EXIT_FAILURE);
        }

        //write outstr into the file
            // Write data to file
            pthread_mutex_lock(&mutex_read_write);
            if(write(arg->file_fd, outstr, strlen(outstr))==-1)
            {
                syslog(LOG_INFO,"Timestamp write has failed");
                pthread_mutex_unlock(&mutex_read_write);
                continue;
            }
            else
            {
                syslog(LOG_INFO, "Syncing data to the disk");
                fdatasync(arg->file_fd);
            }
            pthread_mutex_unlock(&mutex_read_write);
    }
    return NULL;
}

#endif
/**
 * @brief Creates a daemon process
 *
 * Performs the standard Unix daemon creation process:
 * 1. Forks and exits parent
 * 2. Creates new session
 * 3. Changes working directory to root
 * 4. Redirects standard file descriptors to /dev/null
 *
 * @return true if daemon creation successful, false otherwise
 */
bool create_daemon()
{
    pid_t pid;
    pid = fork();
    bool status = false;
    int dev_null_fd;

    if (pid < 0)
    {
        syslog(LOG_ERR, "Fork failed for Daemon");
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
        syslog(LOG_ERR, "New Session Creation failed for Daemon");
        return status;
    }

    // Change the working directory to "/"
    if (chdir("/") == -1)
    {
        syslog(LOG_ERR, "Changing working directory failed for Daemon");
        return status;
    }
    // Since no files were open in parent, no fds are closed here
    //  Redirect STDIN , STDOUT and STDERR to /dev/null
    dev_null_fd = open("/dev/null", O_RDWR);
    if (dev_null_fd == -1)
    {
        LOG("Failed to open /dev/null");
        return status;
    }

    // Redirect stdin (fd 0) to /dev/null
    if (dup2(dev_null_fd, STDIN_FILENO) == -1)
    {
        LOG("Failed to redirect stdin");
        close(dev_null_fd);
        return status;
    }

    // Redirect stdout (fd 1) to /dev/null
    if (dup2(dev_null_fd, STDOUT_FILENO) == -1)
    {
        LOG("Failed to redirect stdout");
        close(dev_null_fd);
        return status;
    }

    // Redirect stderr (fd 2) to /dev/null
    if (dup2(dev_null_fd, STDERR_FILENO) == -1)
    {
        LOG("Failed to redirect stderr");
        close(dev_null_fd);
        return status;
    }

    // Close the original /dev/null file descriptor
    close(dev_null_fd);
    return true;
}

/**
 * @brief Cleans up resources before program exit
 *
 * Closes all open file descriptors, truncates and removes the socket data file,
 * closes syslog, and performs other necessary cleanup operations
 */
void clean()
{
    
    close(sockfd);
    close(client_fd);
    ftruncate(file_fd, 0);
    close(file_fd);
    closelog();
    syslog(LOG_INFO, "Cleaning and Closing the aesdsocket");
    exit(0);
}

/**
 * @brief Signal handler for SIGTERM and SIGINT
 *
 * Sets the exit flag to true when either SIGTERM or SIGINT is received,
 * allowing for graceful program termination
 *
 * @param signo The signal number received
 */
void signal_handler(int signo)
{
    if ((signo == SIGINT) || (signo == SIGTERM))
    {
        exit_flag = true;
        syslog(LOG_DEBUG, "Caught signal, exiting");
        
    }
}

/**
 * @brief Registers signal handlers for SIGTERM and SIGINT
 *
 * Sets up signal handling using sigaction for proper signal management
 */
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

/**
 * @brief Receives data from client and writes to file
 *
 * Receives data from the client socket until a newline is found,
 * dynamically resizing the buffer as needed. Writes received data
 * to the specified file descriptor.
 *
 * @param client_fd The client socket file descriptor
 * @param file_fd The file descriptor to write data to
 * @return Number of bytes written on success, -1 on failure
 */
int send_rcv_socket_data(int client_fd, int file_fd)
{
    char *client_buffer = NULL;
    size_t total_received = 0;
    size_t current_size = CLIENT_BUFFER_LEN;

    // Allocate initial buffer
    client_buffer = (char *)calloc(current_size, sizeof(char));
    if (client_buffer == NULL)
    {
        LOG("[-] calloc");
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
                LOG("[-] recv");
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
                LOG("[-] realloc");
                syslog(LOG_ERR, "ERROR: Buffer reallocation failed");
                free(client_buffer);
                return -1;
            }
            client_buffer = new_buffer;
            current_size = new_size;
        }
    }
    pthread_mutex_lock(&mutex_read_write);
    // Reset file position to beginning before writing

    #ifndef USE_AESD_CHAR_DEVICE
        if (lseek(file_fd, 0, SEEK_SET) == -1)
        {
            LOG("[-] lseek");
            syslog(LOG_ERR, "ERROR: lseek failed: %s", strerror(errno));
            free(client_buffer);
            return -1;
        }
    #endif
        
        //read(file_fd, client_buffer, sizeof(client_buffer));


    // Write data to file
    size_t written = write(file_fd, client_buffer, total_received);
    if (written == -1 || written != total_received)
    {
        LOG("[-] write");
        syslog(LOG_ERR, "ERROR: File write failed: %s", strerror(errno));
        free(client_buffer);
        return -1;
    }
    ////printf("[+] Written %ld bytes \n", total_received);
    // Ensure data is written to disk
    #ifndef USE_AESD_CHAR_DEVICE
        if (fdatasync(file_fd) == -1)
        {
            LOG("[-] fdatasync");
            syslog(LOG_ERR, "ERROR: fdatasync failed: %s", strerror(errno));
            free(client_buffer);
            return -1;
        }
    #endif

    free(client_buffer);
    pthread_mutex_unlock(&mutex_read_write);
    return written;
}

/**
 * @brief Sends file data back to client
 *
 * Reads data from the specified file and sends it back to the client
 * in chunks. Handles partial sends and interruptions.
 *
 * @param client_fd The client socket file descriptor
 * @param file_fd The file descriptor to read data from
 * @return 0 on success, -1 on failure
 */
int return_socketdata_to_client(int client_fd, int file_fd)
{
    char *send_buffer;
    ssize_t bytes_read;

    pthread_mutex_lock(&mutex_read_write);
 
    // Reset file position to start
    //lseek(file_fd, 0, SEEK_SET);
    //     // For character device, close and reopen to reset position
    #ifdef USE_AESD_CHAR_DEVICE
        close(file_fd);
        file_fd = open(SOCKET_FILE, O_RDWR | O_APPEND | O_CREAT, 0666);
        if (file_fd == -1) {
            LOG("[-] reopen");
            syslog(LOG_ERR, "ERROR: Reopening device failed: %s", strerror(errno));
            pthread_mutex_unlock(&mutex_read_write);
            return -1;
        }
    #else
        // Reset file position to start for regular files
        if (lseek(file_fd, 0, SEEK_SET) == -1) {
            LOG("[-] lseek");
            syslog(LOG_ERR, "ERROR: lseek failed: %s", strerror(errno));
            pthread_mutex_unlock(&mutex_read_write);
            return -1;
        }
    #endif
  

    send_buffer = (char *)malloc(CLIENT_BUFFER_LEN);
    if (send_buffer == NULL)
    {
        LOG("[-] malloc");
        syslog(LOG_ERR, "ERROR : Client buffer Allocation failed");
        return -1;
    }

    // Read and send data
    while ((bytes_read = read(file_fd, send_buffer, CLIENT_BUFFER_LEN)) > 0)
    {
    
        printf("No_OF Bytes read : %ld and data : %s\n",bytes_read, send_buffer);
        ssize_t sent = send(client_fd, send_buffer, bytes_read, 0);
        if (sent == -1)
        {
            if (errno == EINTR)
                continue;
            LOG("[-] send");
            syslog(LOG_ERR, "ERROR: Send to client failed: %s",
                    strerror(errno));
            free(send_buffer);
            return -1;
        }
        ////printf("[+] Send %ld bytes \n", bytes_read);

    }

    free(send_buffer);
    pthread_mutex_unlock(&mutex_read_write);
    return 0;
}

/**
 * @brief Main program entry point
 *
 * Sets up the socket server, handles client connections, and manages
 * the main program loop. Supports daemon mode with -d argument.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return 0 on success, non-zero on failure
 */
int main(int argc, char **argv)
{

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
        LOG("[-] Socket");
        syslog(LOG_ERR, "ERROR : Socket Generation Failed");
        // perror("Socket Generation Failed \n");
        clean();
    }
    syslog(LOG_INFO, "Socket Generation Successfull with sockfd : %d", sockfd);
    ////printf("[+] Listening on port 9000 \n");

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
    ////printf("[+] Getaddr successfull \n");

    syslog(LOG_INFO, "getaddrinfo successfull");

    
    ////printf("[+] Getaddr successfull \n");

    int yes = 1;
    socklen_t size = sizeof(yes);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, size);
    // Bind
    if (bind(sockfd, serverInfo->ai_addr, serverInfo->ai_addrlen) != 0)
    {
        LOG("[-] bind \n");
        syslog(LOG_ERR, "Bind Operation failed");

        // clearing the serverInfo
        if (serverInfo != NULL)
        {
            freeaddrinfo(serverInfo);
        }

        clean();
    }

    syslog(LOG_INFO, "Bind successfull");
    ////printf("[+] Bind successfull \n");

    // Check if daemon to be created
    if (daemon_mode)
    {
        if (!create_daemon())
        {
            syslog(LOG_ERR, "Daemon creation failed, hence exiting");
            freeaddrinfo(serverInfo); // free the linked-list
            clean();
        }
        ////printf("[+] Deamon created \n");
    }

    if (listen(sockfd, 20) == -1)
    {
        syslog(LOG_ERR, "ERROR : Can't Listen");
        clean();
    }
    ////printf("[+] Listening on port 9000 \n");
    syslog(LOG_INFO, "Listeing successfully");


    file_fd = open(SOCKET_FILE, O_RDWR | O_APPEND | O_CREAT, 0666);
    if (file_fd == -1)
    {
        LOG("[-] open");
        syslog(LOG_ERR, "ERROR : Generation of /var/tmp/aesdsocketdata failed");
        freeaddrinfo(serverInfo);
        clean();
        exit(0);
    }

    // register signal handlers
    reg_signal_handler();
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof(client_addr);
    
    // timer thread
    
    #ifndef USE_AESD_CHAR_DEVICE
  
	    thread_arg *timer_t = malloc(sizeof(thread_arg));
	    if (!timer_t) {
		syslog(LOG_ERR, "Failed to allocate thread argument");
		close(client_fd);
	    }

	    timer_t -> client_fd = client_fd;
	    timer_t -> file_fd = file_fd;
	    timer_t -> socket_addr = client_addr;

	    pthread_t timer;
	    syslog(LOG_INFO, "Creating a new thread");
	    int err_t = pthread_create(&timer, NULL, timer_thread, timer_t);
	    if(err_t == 0)
	    {
		//printf("[+] Timer Thread creation Successfull \n");
	    }
	    else{
		//printf("[-] Thread creation Failed \n");
		syslog(LOG_ERR, "Error creating timer thread: %s", strerror(err_t));
		free(timer_t);
		  // because we are accepting connection infinite times and seperating it to      multiple         threads 
	    }
    
    #endif


    while (!exit_flag)
    {
        client_fd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_size);
        if ((client_fd) == -1)
        {
            LOG("[-] accpet");
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
        //printf("\n");
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
       
        ////printf("[+] accepted client ip: %s\n", client_ip);

        // Creating a thread for accepted connection

        thread_arg *t_args = malloc(sizeof(thread_arg));
        if (!t_args) {
            syslog(LOG_ERR, "Failed to allocate thread argument");
            close(client_fd);
            continue;
        }

        t_args -> client_fd = client_fd;
        t_args -> file_fd = file_fd;
        t_args -> socket_addr = client_addr;

        pthread_t thread_id;
        syslog(LOG_INFO, "Creating a new thread");
        int err = pthread_create(&thread_id, NULL, thread_operation, t_args);
        if(err == 0)
        {
            //printf("[+] Thread creation Successfull \n");
        }
        else{
            //printf("[-] Thread creation Failed \n");
            syslog(LOG_ERR, "Error creating thread: %s", strerror(err));
            free(t_args);
            close(client_fd);
            continue;           // because we are accepting connection infinite times and seperating it to      multiple         threads 

        }

        thread_node_add(thread_id);
        thread_join();
        

        // int written_bytes;
        // // Receive packets from the client and store in SOCKETDATA_FILE
        // if ((written_bytes = send_rcv_socket_data(client_fd, file_fd)) > 0)
        // {
        //     // Send back the stored data of file back to the client
        //     return_socketdata_to_client(client_fd, file_fd);
        // }
        
    }
    thread_join();
    // if (pthread_join(timer, NULL) != 0) {
    //     syslog(LOG_ERR, "Failed to join timer thread");
    // }
    #ifndef USE_AESD_CHAR_DEVICE
        free(timer_t);
    #endif
    freeaddrinfo(serverInfo);
    clean();
    
}

