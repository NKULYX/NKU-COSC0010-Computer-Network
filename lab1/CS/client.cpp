//
// Created by Lenovo on 2022/10/17.
//

#include "util.h"

SOCKET clientSocket;
SOCKADDR_IN clientAddress;
SOCKADDR_IN serverAddress;
unsigned short clientPort;
std::string userName;
std::string serverIP;

bool online = true;


[[noreturn]] DWORD WINAPI recvProc(LPVOID lparam);
void sendUserVerify();
bool sendProc();

int main(){
    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cout<< "WSAStartup failed!" << std::endl;
    }
    else {
        std::cout << "WSAStartup success!" << std::endl;
    }

    // create client socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Create socket failed! Failure code: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Create socket success!" << std::endl;
    }

    // set client IP address
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");
    // get client port
    std::cout << "Please Input your port: ";
    std::cin >> clientPort;
    clientAddress.sin_port = htons(clientPort);

    // get username
    std::cout << "Please Input your username: ";
    std::cin >> userName;

    // bind
    if (bind(clientSocket, (SOCKADDR*)&clientAddress, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        std::cout << "Bind failed! Failure code: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Bind success!" << std::endl;
    }

    // set server IP address
    serverAddress.sin_family = AF_INET;
    std::cout << "Please Input server IP: ";
    std::cin >> serverIP;
    serverAddress.sin_addr.S_un.S_addr = inet_addr(serverIP.c_str());
    serverAddress.sin_port = htons(10086);

    // connect
    if (connect(clientSocket, (SOCKADDR*)&serverAddress, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        std::cout << "Connect failed! Failure code: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Connect success!" << std::endl;
    }


    HANDLE recvThreadHandle = CreateThread(NULL, (SIZE_T) NULL, recvProc, (LPVOID) clientSocket, 0, 0);

    sendUserVerify();

    while(sendProc());

    CloseHandle(recvThreadHandle);
    closesocket(clientSocket);
    WSACleanup();

}

/**
 * Thread for receiving message
 * @param lparam ThreadParam
 * @return
 */
[[noreturn]] DWORD WINAPI recvProc(LPVOID lparam){
    auto socket = (SOCKET) (LPVOID) lparam;
    while(online) {
        struct Message msg;
        int recvLen = recv(socket, (char *) &msg, sizeof(struct Message), 0);
        if(recvLen > 0){
            printMessage(msg);
            std::cout << "============================ SEND ============================" << std::endl;
        }else{
            std::cout << "Receive failed" << std::endl;
        }
    }
}

/**
 * send user verify message to server
 */
void sendUserVerify(){
    struct Message msg;
    msg.type = MessageType::VERIFY;
    msg.time = time(nullptr);
    msg.toAll = true;
    strcpy(msg.fromUsername, userName.c_str());
    int sendLen = send(clientSocket, (char *) &msg, sizeof(struct Message), 0);
    if(sendLen > 0){
        std::cout << "Send success" << std::endl;
    }else{
        std::cout << "Send failed" << std::endl;
    }
}

/**
 * Send message
 * @return
 */
bool sendProc(){
    std::cout << "============================ SEND ============================" << std::endl;
    struct Message msg;
    msg.time = time(nullptr);
    strcpy(msg.fromUsername, userName.c_str());
    std::string option;
    std::cin >> msg.message;
    if(std::string(msg.message) == "EXIT"){
        msg.type = MessageType::EXIT;
        msg.toAll = true;
        online = false;
    } else{
        msg.type = MessageType::TEXT;
        std::cin >> option;
        if(option == "-a"){
            msg.toAll = true;
        } else{
            msg.toAll = false;
            // 删掉option的前两个字符
            option = option.erase(0, 3);
            strcpy(msg.toUsername, option.c_str());
        }
    }
    int sendLen = send(clientSocket, (char *) &msg, sizeof(struct Message), 0);
    if(sendLen > 0){
        std::cout << "Send success" << std::endl;
    }else{
        std::cout << "Send failed" << std::endl;
    }
    std::cout << "============================ SEND ============================" << std::endl;
    if(std::string(msg.message) == "EXIT"){
        Sleep(500);
        return false;
    } else{
        return true;
    }
}