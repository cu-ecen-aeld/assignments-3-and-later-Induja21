#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg, ...) printf("threading DEBUG: " msg "\n", ##__VA_ARGS__)
// #define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg, ...) printf("threading ERROR: " msg "\n", ##__VA_ARGS__)

#define SUCCESS 0
void *threadfunc(void *thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    // struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    int ret_status = -1;
    // Wait for thread_func_args->wait_to_obtain_ms
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    ret_status = pthread_mutex_lock (thread_func_args->mutex);
    if(ret_status != SUCCESS)
    {
        thread_func_args->thread_complete_success = false;
        ERROR_LOG("pthread acquiring mutex failed. Error reason: %s",strerror(ret_status));
    }
    else
    {
        usleep(thread_func_args->wait_to_release_ms * 1000);
        ret_status = pthread_mutex_unlock (thread_func_args->mutex);
        if(ret_status != SUCCESS)
        {
            thread_func_args->thread_complete_success = false;
            ERROR_LOG("pthread releasing mutex failed. Error reason: %s",strerror(ret_status));
            
        }
        else
        {
            thread_func_args->thread_complete_success = true;
            DEBUG_LOG("pthread execution is successful");
        }
        
    }

    
    return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    bool thread_creation_success = false;
    int pthread_create_status;
    struct thread_data *thread_data = (struct thread_data *)malloc(sizeof(struct thread_data));
    if(thread_data==NULL)
    {
        return thread_creation_success;
    }

    thread_data->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_data->wait_to_release_ms = wait_to_release_ms;
    thread_data->mutex = mutex;
    pthread_create_status = pthread_create(thread, NULL, threadfunc, thread_data);
    if (pthread_create_status== SUCCESS)
    {
        DEBUG_LOG("pthread creation is successful");
        thread_creation_success = true;
    }
    else
    {
        thread_creation_success = false;
        free(thread_data);
        ERROR_LOG("pthread creation failed. Error reason: %s",strerror(pthread_create_status));
        
    }


    return thread_creation_success;
}
