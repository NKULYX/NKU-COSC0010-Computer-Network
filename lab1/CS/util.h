//
// Created by Lenovo on 2022/10/17.
//

#ifndef CS_UTIL_H
#define CS_UTIL_H

#include <iostream>
#include <string>
#include <ctime>
#include <winsock.h>

struct IP{
    std::string IPAddress;
    unsigned short port;
};

std::string IP2Str(const struct IP& ip){
    std::string userIP = ip.IPAddress + ":" + std::to_string(ip.port);
    return userIP;
}

struct IP str2IP(const std::string& str){
    struct IP ip;
    int pos = str.find(':');
    ip.IPAddress = str.substr(0,pos);
    ip.port = std::stoi(str.substr(pos+1));
    return ip;
}

enum class MessageType{
    Verify,
    Text,
    EXIT
};

struct Message{
    MessageType type;
    time_t time;
    std::string fromUsername;
    struct IP fromIP;
    std::string toUsername;
    struct IP toIP;
    std::string message;
};

//void printMessage(struct Message message)
//{
//    printf("=========================== Recv ===========================\n");
//    char timeBuff[50];
//    ctime_s(timeBuff, 50, &message.time);
//    printf("From IPv4 address : %d.%d.%d.%d:%d\n",message.fromIP.ip_1,message.fromIP.ip_2,message.fromIP.ip_3,message.fromIP.ip_4,message.fromIP.port);
//    printf("Send time : %s", timeBuff);
//    time_t recTime;
//    time(&recTime);
//    ctime_s(timeBuff, 50, &recTime);
//    printf("Recv time : %s", timeBuff);
//    printf("Message : %s\n", message.message);
//    printf("=========================== Recv ===========================\n\n");
//    printf("=========================== Send ===========================\n");
//}

#endif //CS_UTIL_H
