#include <chrono>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex gLock;
std::condition_variable gConditionVariable;

int main(){
    
    int result = 0;
    bool notified = false;

    //reporting thread
    //MUST WAI ON WORK, done by the working thread
    std::thread reporter([&]{
        std::unique_lock<std::mutex> lock(gLock);

        if(not notified){
            gConditionVariable.wait(lock);    
        }

        std::cout << "reporter: result is: " << result << std::endl;
    });

    //working thread
    std::thread worker([&]{
        std::unique_lock<std::mutex> lock(gLock);
        //do our work, because we have the lock
        result = 42 + 1 + 3;
        
        //our work is done
        notified = true;    
        
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "work complete" << std::endl;  
        // wake up a thread, that is waiting
        gConditionVariable.notify_one();
    });

    reporter.join();
    worker.join();

    return 0;
}