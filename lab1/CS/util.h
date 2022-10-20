//
// Created by Lenovo on 2022/10/17.
//

#ifndef CS_UTIL_H
#define CS_UTIL_H

#include <iostream>
#include <string>
#include <ctime>
#include <winsock.h>

#define MAX_LEN 500

struct IP{
    char IPAddress[16]{};
    unsigned short port{};
};

std::string IP2Str(const struct IP& ip){
    std::string userIP = std::string(ip.IPAddress) + ":" + std::to_string(ip.port);
    return userIP;
}

struct IP str2IP(const std::string& str){
    struct IP ip;
    int pos = str.find(':');
    std::string ipStr = str.substr(0,pos);
    strcpy(ip.IPAddress,ipStr.c_str());
    ip.port = std::stoi(str.substr(pos+1));
    return ip;
}

enum class MessageType{
    VERIFY,
    TEXT,
    EXIT
};

struct Message{
    MessageType type{};
    bool toAll{};
    time_t time{};
    char fromUsername[15]{};
    struct IP fromIP{};
    char toUsername[15]{};
    struct IP toIP{};
    char message[MAX_LEN]{};
};

void printMessage(const struct Message& message)
{
    MessageType type = message.type;
    char timeBuff[32]{0};
    ctime_s(timeBuff, 50, &message.time);
    switch(type){
        case MessageType::VERIFY:{
            std::cout << "=========================== VERIFY ===========================" << std::endl;
            std::cout << "Time: " << timeBuff;
            std::cout << "User: " << message.fromUsername << std::endl;
            std::cout << "IP: " << IP2Str(message.fromIP) << std::endl;
            std::cout << "Join in !" << std::endl;
            std::cout << "=========================== VERIFY ===========================" << std::endl;
            break;
        }
        case MessageType::TEXT: {
            std::cout << "============================ TEXT ============================" << std::endl;
            std::cout << "Time: " << timeBuff;
            std::cout << "From: " << message.fromUsername << std::endl;
            std::cout << "IP: " << IP2Str(message.fromIP) << std::endl;
            if(message.toAll){
                std::cout << "To: All" << std::endl;
            } else{
                std::cout << "To: " << message.toUsername << std::endl;
                std::cout << "IP: " << IP2Str(message.toIP) << std::endl;
            }
            std::cout << "Message: " << message.message << std::endl;
            std::cout << "============================ TEXT ============================" << std::endl;
            break;
        }
        case MessageType::EXIT: {
            std::cout << "============================ EXIT ============================" << std::endl;
            std::cout << "Time: " << timeBuff;
            std::cout << "User: " << message.fromUsername << std::endl;
            std::cout << "IP: " << IP2Str(message.fromIP) << std::endl;
            std::cout << "Exit !" << std::endl;
            std::cout << "============================ EXIT ============================" << std::endl;
            break;
        }
    }
}

#endif //CS_UTIL_H
