//
// Created by Lenovo on 2022/10/17.
//

#include "util.h"
#include <map>

struct ThreadParam{
    SOCKET socket;
    SOCKADDR_IN address;
};

SOCKET serverSocket;
SOCKADDR_IN serverAddress;
std::map<std::string,std::string> usernameToIP;
std::map<std::string,SOCKET> connections;
std::map<std::string,HANDLE> recvHandlers;


[[noreturn]] DWORD WINAPI listenProc(LPVOID lparam);
[[noreturn]] DWORD WINAPI recvProc(LPVOID lparam);
boolean shutdown();
void processConnection(SOCKET socketConnect, SOCKADDR_IN fromAddress);
void processMessage(struct Message& message, SOCKADDR_IN fromAddress);
void userVerify(struct Message& message);
void broadcastMessage(struct Message& message);
void userExit(struct Message& message);

void cleanupResources();



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

    // create server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Create socket failed! Failure code: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Create socket success!" << std::endl;
    }

    // set server IP address 0.0.0.0 and port
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.S_un.S_addr = inet_addr("0.0.0.0");
    serverAddress.sin_port = htons(10086);

    // bind
    if (bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        std::cout << "Bind failed! Failure code: " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "Bind success!" << std::endl;
    }

    // begin listening
    if(listen(serverSocket, 5) < 0){
        std::cout << "Listen failed!" << std::endl;
    }
    else {
        std::cout << "Listen success!" << std::endl;
    }

    HANDLE listenThreadHandle = CreateThread(NULL, (SIZE_T) NULL, listenProc, (LPVOID) serverSocket, 0, 0);

    while(shutdown());

    cleanupResources();
    CloseHandle(listenThreadHandle);
    closesocket(serverSocket);
    WSACleanup();

}

/**
 * wait for server shutdown
 * @return
 */
boolean shutdown() {
    std::string cmd;
    std::cin >> cmd;
    if(cmd == "EXIT"){
        return false;
    }else{
        return true;
    }
}

/**
 * Thread for listening the connection
 * @param lparam server socket
 * @return
 */
[[noreturn]] DWORD WINAPI listenProc(LPVOID lparam){
    auto serverSock = (SOCKET) (LPVOID) lparam;
    int addressLen = sizeof(SOCKADDR_IN);
    while(true) {
        SOCKADDR_IN fromAddress;
        SOCKET socketConnect = accept(serverSock, (SOCKADDR *)&fromAddress, &addressLen);
        // deal with the connection
        processConnection(socketConnect, fromAddress);
    }
}

/**
 * deal with the connection
 * @param socketConnect socket connected to the server
 * @param fromAddress connection address
 */
void processConnection(SOCKET socketConnect, SOCKADDR_IN fromAddress) {
    auto* threadParam = new ThreadParam();
    threadParam->socket = socketConnect;
    threadParam->address = fromAddress;
    HANDLE recvThreadHandle = CreateThread(NULL, (SIZE_T) NULL, recvProc, (LPVOID) threadParam, 0, 0);
    // get IP from the client connected
    struct IP fromIP;
    strcpy(fromIP.IPAddress,inet_ntoa(fromAddress.sin_addr));
    fromIP.port = ntohs(fromAddress.sin_port);
    std::string userIP = IP2Str(fromIP);
    // insert the handler and connection into the map
    recvHandlers.insert({userIP, recvThreadHandle});
    connections.insert({userIP, socketConnect});
    std::cout << "[CONNECT_LOG] : {IP : " << userIP << "} connected!" << std::endl;
}

/**
 * Thread for receiving message
 * @param lparam ThreadParam
 * @return
 */
[[noreturn]] DWORD WINAPI recvProc(LPVOID lparam){
    auto threadParam = (struct ThreadParam *) (LPVOID) lparam;
    auto socketConnect = threadParam->socket;
    auto fromAddress = threadParam->address;
    while(true) {
        struct Message msg;
        int recvLen = recv(socketConnect, (char *) &msg, sizeof(struct Message), 0);
        if(recvLen > 0){
            processMessage(msg, fromAddress);
            if(msg.type == MessageType::EXIT){
                break;
            }
        }else{
            std::string fromIP = inet_ntoa(fromAddress.sin_addr);
            std::cout << "[ERROR_LOG] : Receive from: " << fromIP << " failed" << std::endl;
        }
    }
}

/**
 * process the message
 * @param message
 * @param fromAddress
 */
void processMessage(struct Message& message, SOCKADDR_IN fromAddress) {
    // get IP and port from the fromAddress and set it in the message
    std::string fromIP = std::string(inet_ntoa(fromAddress.sin_addr));
    strcpy(message.fromIP.IPAddress, fromIP.c_str());
    message.fromIP.port = ntohs(fromAddress.sin_port);

    if(!message.toAll){
        std::string toUsername = message.toUsername;
        if(usernameToIP.find(toUsername)==usernameToIP.end()){
            std::cout << "[ERROR_LOG] : {User : " << toUsername << "} not found!" << std::endl;
            return;
        }
        std::string toIP = usernameToIP[toUsername];
        message.toIP = str2IP(toIP);
    }

    // deal with different message type
    MessageType type = message.type;
    switch(type){
        case MessageType::VERIFY: {
            userVerify(message);
            break;
        }
        case MessageType::TEXT: {
            break;
        }
        case MessageType::EXIT: {
            userExit(message);
            break;
        }
    }
    printMessage(message);
    broadcastMessage(message);
}


/**
 * user verify : set username to IP
 * @param message
 */
void userVerify(struct Message& message){
    std::string username = message.fromUsername;
    std::string userIP = IP2Str(message.fromIP);
    usernameToIP.insert({username, userIP});
    std::cout << "[VERIFY_LOG] : {User : " << username << "} from {IP : " << userIP << "} join in !" << std::endl;
}

/**
 * broadcast message to all users
 * @param message
 */
void broadcastMessage(struct Message& message){
    if(!message.toAll){
        struct Message newMessage{};
        newMessage.type = MessageType::TEXT;
        newMessage.time = message.time;
        strcpy(newMessage.fromUsername, message.fromUsername);
        newMessage.fromIP = message.fromIP;
        strcpy(newMessage.toUsername, message.toUsername);
        newMessage.toIP = message.toIP;
        SOCKET socketConnect = connections[IP2Str(message.toIP)];
        send(socketConnect, (char *)&message, sizeof(struct Message), 0);
        return ;
    }
    // get IP and send message to all users
    for(const auto& userInfo : usernameToIP){
        if(userInfo.first == std::string(message.fromUsername)){
            continue;
        }
        std::string username = userInfo.first;
        std::string userIP = userInfo.second;
        struct IP toIP = str2IP(userIP);
        struct Message newMessage;
        newMessage.type = message.type;
        newMessage.time = message.time;
        strcpy(newMessage.fromUsername, message.fromUsername);
        newMessage.fromIP = message.fromIP;
        strcpy(newMessage.toUsername, username.c_str());
        newMessage.toIP = toIP;
        strcpy(newMessage.message, message.message);
        SOCKET socket = connections[userIP];
        send(socket, (char *) &newMessage, sizeof(struct Message), 0);
    }
}

/**
 * user exit : remove the user from the map and release the resources
 * @param message
 */
void userExit(struct Message& message){
    std::string username = message.fromUsername;
    std::string userIP = IP2Str(message.fromIP);
    CloseHandle(recvHandlers[userIP]);
    closesocket(connections[userIP]);
    connections.erase(userIP);
    recvHandlers.erase(userIP);
    usernameToIP.erase(username);
    std::cout << "[EXIT_LOG] : {User : " << username << "} from {IP : " << userIP << "} exit !" << std::endl;
}

/**
 * release all the resources
 */
void cleanupResources(){
    // release all the resources
    for(const auto& userInfo : usernameToIP){
        std::string username = userInfo.first;
        std::string userIP = userInfo.second;
        HANDLE recvThreadHandle = recvHandlers[userIP];
        CloseHandle(recvThreadHandle);
        closesocket(connections[userIP]);
    }
}
