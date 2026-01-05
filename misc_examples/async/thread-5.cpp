#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

std::mutex gLock;
static int shared_value = 0;

void test(){
    gLock.lock();
        shared_value += 1;
    gLock.unlock();
}

int main(){    
    std::vector<std::thread> threads; 
    
    for(int i = 0; i < 1000; i++){
        threads.push_back(std::thread(test));
    }
    
    for(int i = 0; i < 1000; i++){
        threads[i].join();
    }

    std::cout << "shared value: " << shared_value << std::endl;
    return 0;
}