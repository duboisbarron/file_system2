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

    pthread_cond_t canRead;
    pthread_cond_t canWrite;
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
        pthread_cond_init(&canRead, NULL);
        pthread_cond_init(&canWrite, NULL);
    }
    
    void reader_enter() {
        /* Your Part II code goes here */
        pthread_mutex_lock(&lock);
        while(activeWriters > 0  || waitingWriters > 0){
            pthread_cond_wait(&canRead, &lock);
        }
        waitingReaders--;
        activeReaders++;

    }

    void reader_exit() {
        /* Your Part II code goes here */
        activeReaders--;
        if(activeReaders == 0 && waitingWriters > 0){
            pthread_cond_signal(&canWrite);
        }

        pthread_mutex_unlock(&lock);
    }
    
    void writer_enter() {
        /* Your Part II code goes here */
        pthread_mutex_lock(&lock);
        waitingWriters++;
        while(activeWriters > 0 || activeReaders > 0){
            pthread_cond_wait(&canWrite, &lock);
        }
        waitingWriters--;
        activeWriters = 1;

    }
    
    void writer_exit() {
        /* Your Part II code goes here */

        activeWriters = 0;
        if(waitingWriters > 0){
            pthread_cond_signal(&canWrite);
        } else if (waitingReaders > 0){
            pthread_cond_broadcast(&canRead);
        }

        pthread_mutex_unlock(&lock);

    }
};
