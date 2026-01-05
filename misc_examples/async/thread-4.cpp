#include <iostream>
#include <thread>
#include <vector>

int main(){
    auto test = [](int x){
        std::cout << "hello from thread! "<< std::this_thread::get_id() << std::endl;
        std::cout << "argument passed in: " << x << std::endl;
    };
    
    // -std=c++20 needed 
    std::vector<std::jthread> jthreads; 
    for(int i = 0; i < 10; i++){
        jthreads.push_back(std::jthread(test, i));
    }

    std::cout << "hello from my main thread" << std::endl;
    return 0;
}