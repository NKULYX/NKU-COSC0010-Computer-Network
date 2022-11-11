//
// Created by Lenovo on 2022/11/11.
//

#include <Util.h>
#include <thread>
#include <fstream>
#include <cmath>

SOCKET serverSocket;
SOCKADDR_IN serverAddress;
std::string serverIP = "127.0.0.1";
unsigned short serverPort = 8888;
SOCKET clientSocket;
SOCKADDR_IN clientAddress;

struct PseudoHeader pseudoHeader{};
const int RTO = 500;
bool ACK_FLAG = false;
int CURRENT_SEQ = 0;

std::string fileDir = "../test/files";

bool serverInit();
bool establishConnection(SOCKET socket);
void sendFiles();
void sendFile(struct FileDescriptor file);
void sendPackage(struct Message message);
void destroyConnection();


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

    while(!serverInit()){
        std::cout << "Server initialization failed! Retry in 1 seconds..." << std::endl;
        Sleep(1000);
    }

    // wait for client connection
    std::cout << "Waiting for client connection..." << std::endl;
    int clientAddressLength = sizeof(SOCKADDR);
    clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddress, &clientAddressLength);

    // initialize pseudo pseudoHeader
    pseudoHeader.sourceIP = clientAddress.sin_addr.S_un.S_addr;
    pseudoHeader.destinationIP = serverAddress.sin_addr.S_un.S_addr;

    // establish connection via three times handshake
    while(!establishConnection(clientSocket)) {
        std::cout << "Establishment failed! Retry in 1 seconds..." << std::endl;
        Sleep(1000);
    }

    // send files
    sendFiles();

    // destroy connection
    destroyConnection();

    // cleanup resources
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

}

/**
 * Initializes the server
 * @return whether initialization is successful
 */
bool serverInit() {
    // set server address
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.S_un.S_addr = inet_addr(serverIP.c_str());
    serverAddress.sin_port = htons(serverPort);

    // create client socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Create socket failed! Failure code: " << WSAGetLastError() << std::endl;
        return false;
    }
    else {
        std::cout << "Create socket success!" << std::endl;
    }

    // bind
    if (bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        std::cout << "Bind failed! Failure code: " << WSAGetLastError() << std::endl;
        return false;
    }
    else {
        std::cout << "Bind success!" << std::endl;
    }

    // begin listening
    if(listen(serverSocket, 5) < 0){
        std::cout << "Listen failed!" << std::endl;
        return false;
    }
    else {
        std::cout << "Listen success!" << std::endl;
    }

    return true;

}

/**
 * Establishes connection with client
 * Need to finish three times handshake
 * @param socket client socket
 * @return whether connection establishment is successful
 */
bool establishConnection(SOCKET socket) {
    // wait for the first handshake
    struct Message recvBuffer{};
    int recvLength = recv(socket, (char*) &recvBuffer, sizeof(struct Message), 0);
    if(recvLength > 0 && recvBuffer.checksumValid(&pseudoHeader) && recvBuffer.isSYN()){
        std::cout << "First handshake received!" << std::endl;
    }
    else {
        std::cout << "First handshake failed!" << std::endl;
        return false;
    }
    // send second handshake
    struct Message sendBuffer{
        serverPort,
        clientAddress.sin_port,
        0,
        recvBuffer.seq
    };
    sendBuffer.setSYN();
    sendBuffer.setACK();
    sendBuffer.setLen(0);
    sendBuffer.setChecksum(&pseudoHeader);
    send(socket, (char*) &sendBuffer, sizeof(struct Message), 0);
    // create a new thread to re-send when timeout
    ACK_FLAG = false;
    int waitTime = 0;
    std::thread resendThread([&](){
        while(!ACK_FLAG){
            while(waitTime < RTO) {
                Sleep(10);
                waitTime += 10;
            }
            send(socket, (char*) &sendBuffer, sizeof(struct Message), 0);
        }
    });
    resendThread.detach();
    // wait for ACK
    while(true) {
        recvLength = recv(socket, (char*) &recvBuffer, sizeof(struct Message), 0);
        if(recvLength > 0 && recvBuffer.checksumValid(&pseudoHeader) && recvBuffer.isACK() && recvBuffer.ack == 0){
            std::cout << "Second handshake received!" << std::endl;
            ACK_FLAG = true;
            break;
        }
        else {
            std::cout << "Second handshake receive failed! Re-send!" << std::endl;
            send(socket, (char*) &sendBuffer, sizeof(struct Message), 0);
            waitTime = 0;
        }
    }
    std::cout << "Three times handshake finished! Connection established!" << std::endl;
    return true;
}

/**
 * send files in the specified directory
 */
void sendFiles() {
    // read files in "test/files" directory
    std::vector<FileDescriptor> files = getAllFiles(fileDir);
    for(auto file : files){
        std::cout << "Send file: " << file.fileName << " size: " << file.fileSize << " bytes begin!" << std::endl;
        // count the time
        auto start = std::chrono::steady_clock::now();
        sendFile(file);
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        std::cout << "Send file: " << file.fileName << " size: " << file.fileSize << " bytes finish in " << duration << "ms !" << std::endl;
    }
}

/**
 * send a file
 * @param file the destination of file
 */
void sendFile(struct FileDescriptor file) {
    // send the description of the file containing the file name and size
    struct Message sendBuffer{
            serverPort,
            clientAddress.sin_port,
            0,
            CURRENT_SEQ
    };
    sendBuffer.setACK();
    sendBuffer.setLen(sizeof(struct FileDescriptor));
    sendBuffer.setData((char*) &file);
    sendBuffer.setChecksum(&pseudoHeader);
    sendPackage(sendBuffer);
    // send file content
    std::ifstream fileStream(fileDir + file.fileName, std::ios::binary);
    // calculate the segment of file aligned by MSS
    int segments = ceil(((double) file.fileSize) / MSS);
    // send file content by segments
    for(int i = 0; i < segments; i++) {
        // read file content
        char fileContent[MSS];
        fileStream.read(fileContent, MSS);
        // send file content
        struct Message fileBuffer{
                serverPort,
                clientAddress.sin_port,
                0,
                CURRENT_SEQ
        };
        fileBuffer.setACK();
        fileBuffer.setLen(fileStream.gcount());
        fileBuffer.setData(fileContent);
        fileBuffer.setChecksum(&pseudoHeader);
        sendPackage(fileBuffer);
    }
}

/**
 * send a package to client
 * @param message message in package
 */
void sendPackage(struct Message message) {
    send(clientSocket, (char*) &message, sizeof(struct Message), 0);
    std::cout << "Send package: [SEQ:" << message.seq << "] [ACK:" << message.ack << "] [Len:" << message.getLen() << "]" << std::endl;
    // create a new thread to re-send when timeout
    ACK_FLAG = false;
    int waitTime = 0;
    std::thread resendThread([&](){
        while(!ACK_FLAG){
            while(waitTime < RTO){
                Sleep(10);
                waitTime += 10;
            }
            send(clientSocket, (char*) &message, sizeof(struct Message), 0);
        }
    });
    resendThread.detach();
    // wait for ACK
    struct Message recvBuffer{};
    while(true) {
        int recvLength = recv(clientSocket, (char*) &recvBuffer, sizeof(struct Message), 0);
        if(recvLength > 0 && recvBuffer.checksumValid(&pseudoHeader)
            && recvBuffer.isACK() && recvBuffer.ack == CURRENT_SEQ){
            std::cout << "Client reply [ACK:" << recvBuffer.ack << "] success!" << std::endl;
            ACK_FLAG = true;
            break;
        }
        else {
            std::cout << "Client reply ACK false! Re-send [SEQ:" << message.seq << "] [ACK:" << message.ack << "]" << std::endl;
            send(clientSocket, (char*) &message, sizeof(struct Message), 0);
            // reset wait time
            waitTime = 0;
        }
    }
    CURRENT_SEQ = ~CURRENT_SEQ;
}

/**
 * destroy connection to client
 * server first send FIN package and wait for ACK
 */
void destroyConnection() {
    struct Message finMsg{
            serverPort,
            clientAddress.sin_port,
            0,
            CURRENT_SEQ
    };
    finMsg.setFIN();
    finMsg.setACK();
    finMsg.setLen(0);
    finMsg.setChecksum(&pseudoHeader);
    sendPackage(finMsg);
    // wait for client FIN package
    struct Message recvBuffer{};
    while(true) {
        int recvLength = recv(clientSocket, (char*) &recvBuffer, sizeof(struct Message), 0);
        if(recvLength > 0 && recvBuffer.checksumValid(&pseudoHeader)
           && recvBuffer.isACK() && recvBuffer.isFIN()){
            std::cout << "Client send FIN package!" << std::endl;
            struct Message replyFin{
                    serverPort,
                    clientAddress.sin_port,
                    0,
                    CURRENT_SEQ
            };
            replyFin.setACK();
            replyFin.setLen(0);
            replyFin.setChecksum(&pseudoHeader);
            sendPackage(replyFin);
            break;
        }
    }
}