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
SOCKADDR_IN clientAddress;
std::string clientIP = "127.0.0.1";
unsigned short clientPort = 8088;

struct PseudoHeader sendPseudoHeader{};
struct PseudoHeader recvPseudoHeader{};
bool ACK_FLAG = false;
int CURRENT_SEQ = 0;

std::string fileDir = "../test/files";

bool serverInit();
void establishConnection();
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

    // establish connection via three times handshake
    establishConnection();

    // send files
    sendFiles();

    // destroy connection
    destroyConnection();

    // cleanup resources
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

    // set client address
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.S_un.S_addr = inet_addr(clientIP.c_str());
    clientAddress.sin_port = htons(clientPort);

    // initialize pseudo sendPseudoHeader
    sendPseudoHeader.sourceIP = serverAddress.sin_addr.S_un.S_addr;
    sendPseudoHeader.destinationIP = clientAddress.sin_addr.S_un.S_addr;
    recvPseudoHeader.sourceIP = clientAddress.sin_addr.S_un.S_addr;
    recvPseudoHeader.destinationIP = serverAddress.sin_addr.S_un.S_addr;

    // create client socket
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
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

    return true;

}

/**
 * Establishes connection with client
 * server first send SYN package and wait for ACK
 */
void establishConnection() {
    struct Message msgSYN{
        serverPort,
        clientPort,
        0,
        CURRENT_SEQ
    };
    msgSYN.setSYN();
    msgSYN.setLen(0);
    msgSYN.setChecksum(&sendPseudoHeader);
    sendPackage(msgSYN);
    std::cout << "[LOG] Connection established!" << std::endl;
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
            clientPort,
            0,
            CURRENT_SEQ
    };
    sendBuffer.setACK();
    sendBuffer.setFHD();
    sendBuffer.setLen(sizeof(struct FileDescriptor));
    sendBuffer.setData((char*) &file);
    sendBuffer.setChecksum(&sendPseudoHeader);
    sendPackage(sendBuffer);
    // send file content
    std::ifstream fileStream(fileDir + "/" + file.fileName, std::ios::binary | std::ios::app);
    // calculate the segment of file aligned by MSS
    int segments = ceil(((double) file.fileSize) / MSS);
    // send file content by segments
    int len = file.fileSize;
    for(int i = 0; i < segments; i++) {
        // read file content
        char fileContent[MSS];
        fileStream.read(fileContent, fmin(len,MSS));
        // send file content
        struct Message fileBuffer{
                serverPort,
                clientPort,
                0,
                CURRENT_SEQ
        };
        fileBuffer.setACK();
        fileBuffer.setLen(fmin(len,MSS));
        fileBuffer.setData(fileContent);
        fileBuffer.setChecksum(&sendPseudoHeader);
        len -= MSS;
        std::cout << "[Seg " << i << " in " << segments << "]" << std::endl;
        sendPackage(fileBuffer);
    }
}

/**
 * send a package to client
 * @param message message in package
 */
void sendPackage(struct Message message) {
    // send package
    if(!randomLoss()){
        sendto(serverSocket, (char*) &message, sizeof(struct Message), 0, (struct sockaddr*) &clientAddress, sizeof(SOCKADDR));
    }
    printMessage(message);
    // enter wait for ACK state
    // first start timer for timeout re-send
    int waitTime = 0;
    ACK_FLAG = false;
    std::thread resendThread([&](){
        // if ACK_FLAG is not set, re-send
        while(!ACK_FLAG){
            while(waitTime < RTO && !ACK_FLAG) {
                Sleep(1);
                waitTime += 1;
            }
            if(!ACK_FLAG) {
                std::cout << "[Timeout] : Re-send Package" << std::endl;
                if(!randomLoss()){
                    sendto(serverSocket, (char*) &message, sizeof(struct Message), 0, (struct sockaddr*) &clientAddress, sizeof(SOCKADDR_IN));
                }
                printMessage(message);
            }
        }
    });
    // then wait for ACK
    while(true) {
        struct Message recvBuffer;
        int clientAddressLength = sizeof(SOCKADDR);
        int recvLength = recvfrom(serverSocket, (char *) &recvBuffer, sizeof(struct Message), 0, (struct sockaddr *) &clientAddress, &clientAddressLength);
        // recvLength > 0 means receive success
        if (recvLength > 0) {
            printMessage(recvBuffer);
            // check checksum and ack
            // only if checksum is valid and ack is for the current seq that the package is sent and received correctly
            if (recvBuffer.checksumValid(&recvPseudoHeader) && recvBuffer.ack == CURRENT_SEQ) {
                ACK_FLAG = true;
                resendThread.join();
                // update current seq
                CURRENT_SEQ = (CURRENT_SEQ + 1) % 2;
                // current send task finish
                std::cout << "[ACK] : Package (SEQ:" << recvBuffer.ack << ") sent successfully!" << std::endl;
                break;
            }
            // if ckecksum is not valid or ack is not for current seq, that means the packet sent just now was failed
            // then wait for timeout to re-send the package
            else {
                std::cout << "[RE] : Client received failed. Wait for timeout to re-send" << std::endl;
            }
        } else {
            std::cout << "[ERROR] : Package received from socket failed!" << std::endl;
        }
    }
}

/**
 * destroy connection to client
 * server first send FIN package and wait for ACK
 */
void destroyConnection() {
    struct Message msgSYN{
            serverPort,
            clientPort,
            0,
            CURRENT_SEQ
    };
    msgSYN.setFIN();
    msgSYN.setLen(0);
    msgSYN.setChecksum(&sendPseudoHeader);
    sendPackage(msgSYN);
    std::cout << "[LOG] Connection destroyed!" << std::endl;
}