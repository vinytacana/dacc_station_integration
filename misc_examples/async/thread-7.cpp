#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

static std::atomic<int> shared_value = 0;

void test(){
    shared_value++;
    // shared_value = shared_value + 1; is not suported
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