//
// Created by Lenovo on 2022/11/11.
//
#include <iostream>
#include <thread>

int main(){
    bool flag = false;
    std::thread t1([&flag](){
        std::cout << "Hello, world! in thread" << std::endl;
        flag = true;
    });
    t1.detach();
    while(!flag);
    std::cout << "Hello, world! in main" << std::endl;
}
