//
// Created by Lenovo on 2022/11/11.
//


#include <Util.h>

#pragma comment(lib, "Ws2_32.lib")

SOCKET clientSocket;
SOCKADDR_IN clientAddress;
std::string clientIP = "127.0.0.1";
unsigned short clientPort = 8088;
SOCKADDR_IN serverAddress;
std::string serverIP = "127.0.0.1";
unsigned short serverPort = 8888;

bool initConnection();
bool establishConnection();

int main() {
    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cout<< "WSAStartup failed!" << std::endl;
    }
    else {
        std::cout << "WSAStartup success!" << std::endl;
    }

    // initialize connection to server
    while(!initConnection()){
        std::cout << "Connection failed! Retry in 1 seconds..." << std::endl;
        Sleep(1000);
    }

    // shake hands with server



}

/**
 * initialize connection to server
 * @return whether connection is successful
 */
bool initConnection() {
    // set client address
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.S_un.S_addr = inet_addr(clientIP.c_str());
    clientAddress.sin_port = htons(clientPort);

    // set server address
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.S_un.S_addr = inet_addr(serverIP.c_str());
    serverAddress.sin_port = htons(serverPort);

    // create client socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Create socket failed! Failure code: " << WSAGetLastError() << std::endl;
        return false;
    }
    else {
        std::cout << "Create socket success!" << std::endl;
    }

    // connect to server
    if (connect(clientSocket, (SOCKADDR*)&serverAddress, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        std::cout << "Connect failed! Failure code: " << WSAGetLastError() << std::endl;
        return false;
    }
    else {
        std::cout << "Connect success!" << std::endl;
    }

    return true;
}