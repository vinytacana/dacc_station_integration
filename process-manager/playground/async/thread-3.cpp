#include <iostream>
#include <thread>
#include <vector>

int main(){
    auto test = [](int x){
        std::cout << "hello from thread! "<< std::this_thread::get_id() << std::endl;
        std::cout << "argument passed in: " << x << std::endl;
    };
    
    std::vector<std::thread> threads; 
    for(int i = 0; i < 10; i++){
        threads.push_back(std::thread(test, i));
    }
    
    for(int i = 0; i < 10; i++){
        threads[i].join();
    }

    std::cout << "hello from my main thread" << std::endl;
    return 0;
}