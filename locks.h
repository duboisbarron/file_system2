#include <pthread.h>


class a4_rwlock {
 private:
    /* example of defining integers:
     *     int num_reader, num_writer;
     * example of defining conditional variables:
     *     pthread_cond_t contitional_variable;
     */
    /* Your Part II code goes here */
    
 public:
    a4_rwlock(){
        /* initializing the variables 
         * example: num_reader = num_writer = 0
         */
        /* Your Part II code goes here */
    }
    
    void reader_enter() {
        /* Your Part II code goes here */
    }

    void reader_exit() {
        /* Your Part II code goes here */
    }
    
    void writer_enter() {
        /* Your Part II code goes here */
    }
    
    void writer_exit() {
        /* Your Part II code goes here */
    }
};
