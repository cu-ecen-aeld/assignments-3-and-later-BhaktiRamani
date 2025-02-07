#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

 
// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    // Waiting for certain time
    int wait_to_obtain_ms = thread_func_args->wait_to_obtain_ms;
    char command[100];
    snprintf(command, sizeof(command), "./sleep.sh %d", wait_to_obtain_ms);
    int delay = system(command);

   //Initializing mutex
    pthread_mutex_init(thread_func_args->mutex, NULL);
    // Obtained mutex
    pthread_mutex_lock(thread_func_args->mutex); 

    // Release wait time
    int wait_to_release_ms = thread_func_args->wait_to_release_ms;
    char command[100];
    snprintf(command, sizeof(command), "./sleep.sh %d", wait_to_release_ms);
    int delay = system(command);

    // Release mutex
    pthread_mutex_unlock(thread_func_args -> mutex); 

    thread_func_args->thread_complete_success = true;
    return thread_param;

    // Configuring mutex to pass into next function
    // thread_func_args -> mutex = PTHREAD_MUTEX_INITIALIZER;
    // pthread_t tid = thread_func_args -> thread;

    //Create that thread
    // pthread_create(&tid, NULL, some_func, NULL);
    // //Join that thread
    // pthread_join(&tid, NULL);
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    //Allocating memory for thread data dynamically
    struct thread_data *data = malloc(sizeof(struct thread_data));
    if (!data) {
        ERROR_LOG("Failed to allocate memory for thread_data");
        return false;
    }

        // Initialize thread_data
    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;

        // Create the thread
    if (pthread_create(thread, NULL, threadfunc, data) != 0) {
        ERROR_LOG("Failed to create thread");
        free(data);
        return false;
    }
    
    return true;





  
}

