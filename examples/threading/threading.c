#include "threading.h"
#include <unistd.h> //for usleep
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
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    struct thread_data* data = (struct thread_data *) thread_param;

    // sleep for the specified time before obtaining the mutex.
    usleep(data->wait_to_obtain_ms * 1000);

    // attemp to obtain the mutex
    if (pthread_mutex_lock(data->mutex) != 0) {
        data->thread_complete_success = false;
        ERROR_LOG("function %s at line %d: failed to lock the mutex", __func__, __LINE__);
        return data;
    }

    // Hold the mutex for the specified time
    usleep(data->wait_to_release_ms * 100);

    // Attempt to release the mutex
    if (pthread_mutex_unlock(data->mutex) != 0) {
        data->thread_complete_success = false;
        ERROR_LOG("function %s at line %d: failed to unlock the mutex", __func__, __LINE__);
        return data;
    }

    data->thread_complete_success = true;
    return thread_param;
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
    struct thread_data *data = (struct thread_data *)malloc(sizeof(struct thread_data));
    if (data == NULL) {
        ERROR_LOG("function %s at line %d: failed to allocate memory for thread data", __func__, __LINE__);
        return false;
    }

    // Initialize thread data
    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;

    //  Create the thread
    int result = pthread_create(thread, NULL, threadfunc, data);
    if (result != 0) {
        ERROR_LOG("function %s at line %d: failed to create the thread", __func__, __LINE__);
        free(data);
        return false;
    }

    return true;
}

