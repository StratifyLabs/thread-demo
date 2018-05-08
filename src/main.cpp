
#include <cstdio>
#include <sapi/sys.hpp>
#include <sapi/chrono.hpp> //use this for delays and timers


static Mutex stdout_mutex;
static void * thread_worker(void * args);


int main(int argc, char * argv[]){
    Cli cli(argc, argv);
    cli.set_publisher("Stratify Labs, Inc");
    cli.handle_version();
    volatile int value = 0; //mark as volatile because it is modified it a different context
    Timer thread_timer;

    Task task_manager; //this class uses /dev/sys to see what processes and threads are running
    TaskInfo task_info;

    //The Thread class wraps the POSIX threads API in an embedded friendly way: https://stratifylabs.co/StratifyAPI/html/classsys_1_1_thread.html
    Thread thread(2048); //create a thread object with a 2KB stack -- heap is shared with main thread (process thread)
    thread.create(thread_worker); //passes a null argument and uses default priority

    stdout_mutex.lock();

    //get info for the main process thread
    task_info = task_manager.get_info(pthread_self());

    if( thread.id() > 0 ){
        //prints the tasks for this process
        task_manager.print(Thread::get_pid());
    }  else {
        printf("Failed to create thread (%d)\n", thread.error_number());
    }

    stdout_mutex.unlock();

    //wait for thread to finish
    while( thread.is_running() ){
        Timer::wait_milliseconds(1);
    }

    //to resuse the thread object for creating a new thread, it needs to be reset
    thread.reset();

    //now make the thread joinable -- this means another thread can block until it completes
    thread.set_detachstate(Thread::JOINABLE);
    thread.set_stacksize(4096); //adjust the stack size

    thread.create(thread_worker, (void*)&value);
    thread_timer.start();

    if( thread.id() > 0 && thread.is_joinable() ){

        //take a look at the task table for this process
        task_manager.print(Thread::get_pid());

        //this won't return until thread has completed and is finished
        printf("Value starts as %d\n", value);
        printf("Is thread still running before join: %d\n", thread.is_running());
        Thread::join(thread.id());
        thread_timer.stop();
        printf("Is thread still running after join: %d\n", thread.is_running());
        printf("Value was modified by thread %d\n", value);
        printf("Thread timer is %ldms (%ldus)\n", thread_timer.milliseconds(), thread_timer.microseconds());
    }



    return 0;
}



void * thread_worker(void * args){
    int * value = 0;
    if( args == 0 ){
        stdout_mutex.lock();
        printf("thread:No Arguments\n");
        stdout_mutex.unlock();
    } else {
        Timer::wait_milliseconds(500);
        value = (int*)args;
        *value = 100;
    }


    return args;
}
