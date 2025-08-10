#include <iostream>
#include <thread>

int main(){
    auto test = [](int x){
        std::cout << "hello from thread! " << std::endl;
        std::cout << "argument passed in: " << x << std::endl;
    };
    
    std::thread myThread(test, 100);
    myThread.join();
    
    std::cout << "hello from my main thread" << std::endl;
    return 0;
}