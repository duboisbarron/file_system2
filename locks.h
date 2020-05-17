#include <pthread.h>


class a4_rwlock {
 private:
    /* example of defining integers:
     *     int num_reader, num_writer;
     * example of defining conditional variables:
     *     pthread_cond_t contitional_variable;
     */
    /* Your Part II code goes here */

    int waitingWriters;
    int waitingReaders;
    int activeWriters;
    int activeReaders;


    pthread_mutex_t lock;
//    pthread_mutex_init(&lock, NULL);
    
 public:
    a4_rwlock(){
        /* initializing the variables 
         * example: num_reader = num_writer = 0
         */
        /* Your Part II code goes here */
        waitingReaders = 0;
        waitingWriters = 0;
        activeReaders = 0;
        activeWriters = 0;

        pthread_mutex_init(&lock, NULL);
    }
    
    void reader_enter() {
        /* Your Part II code goes here */
        pthread_mutex_lock(&lock);

    }

    void reader_exit() {
        /* Your Part II code goes here */

        pthread_mutex_unlock(&lock);
    }
    
    void writer_enter() {
        /* Your Part II code goes here */
        pthread_mutex_lock(&lock);
    }
    
    void writer_exit() {
        /* Your Part II code goes here */
        pthread_mutex_unlock(&lock);

    }
};
