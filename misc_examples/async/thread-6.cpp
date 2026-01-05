//DEADLOCKS

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

std::mutex gLock;
static int shared_value = 0;

// ! bad code, deadlock on throw
// void test(){
//     gLock.lock();
//         try{
//             shared_value += 1;
//             throw "dangerous...abort.";
//         }catch(...){
//             std::cout << "handle exception";
//             return;
//         }
//     gLock.unlock();
// }

void test(){
    std::lock_guard<std::mutex> lockGuard(gLock);

    try{
        shared_value += 1;
        throw "dangerous...abort.";
    }catch(...){
        std::cout << "handle exception";
        return;
    }
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
