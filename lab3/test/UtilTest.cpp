//
// Created by Lenovo on 2022/11/11.
//

#include <Util.h>

int main(){
    std::string ip = "127.0.0.1";
    struct PseudoHeader sendHeader{};
    sendHeader.sourceIP = inet_addr(ip.c_str());
    sendHeader.destinationIP = inet_addr(ip.c_str());
    sendHeader.length = sizeof(struct Message);
    struct Message sendMsg{};
    sendMsg.sourcePort = 8080;
    sendMsg.destinationPort = 8081;
    sendMsg.ack = 0;
    sendMsg.seq = 0;
    sendMsg.setSYN();
    std::string data = "Hello, world!";
    strcpy(sendMsg.data, data.c_str());
    sendMsg.setLen(data.length());
    sendMsg.setChecksum(&sendHeader);

    struct Message recvMsg{};
    memcpy(&recvMsg, &sendMsg, sizeof(struct Message));
    recvMsg.setACK();

    std::cout << sendMsg.checksumValid(&sendHeader) << std::endl;
    std::cout << recvMsg.getLen() << std::endl;
    std::cout << recvMsg.checksumValid(&sendHeader) << std::endl;
}
