//
// Created by Lenovo on 2022/11/11.
//

#include <Util.h>
#include <fstream>
#include <cmath>
#include <deque>

SOCKET serverSocket;
SOCKADDR_IN serverAddress;
std::string serverIP = "127.0.0.1";
unsigned short serverPort = 8888;
SOCKADDR_IN clientAddress;
std::string clientIP = "127.0.0.1";
unsigned short clientPort = 8088;

struct PseudoHeader sendPseudoHeader{};
struct PseudoHeader recvPseudoHeader{};

std::deque<struct Message> sendBuffer;
bool ACK_FLAG = false;
int baseSeq = 0;
int nextSeq = 0;
const int WINDOW_SIZE = 16;
Timer timer;
Logger logger;
std::mutex bufferMutex;

std::string fileDir = "../test/tmp";

bool serverInit();
void establishConnection();
void beginRecv();
void beginTimeOut();
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

    // begin to receive packets from client
    beginRecv();

    // begin to check timeout to re-send
    beginTimeOut();

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
 * begin to receive from client
 */
void beginRecv() {
    std::thread recvThread([&](){
        while(true) {
            struct Message recvBuffer;
            int clientAddressLength = sizeof(SOCKADDR);
            int recvLength = recvfrom(serverSocket, (char *) &recvBuffer, sizeof(struct Message), 0, (struct sockaddr *) &clientAddress, &clientAddressLength);
            // if receive buffer is valid
            if(recvLength > 0) {
                if(recvBuffer.checksumValid(&recvPseudoHeader)) {
                    logger.addLog("[RECV] : " + message2string(recvBuffer));
                    if(recvBuffer.isSYN() && recvBuffer.isACK()) {
                        logger.addLog("[LOG] Connection established!");
                    }
                    else if(recvBuffer.isFIN() && recvBuffer.isACK()) {
                        logger.addLog("[LOG] Connection destroyed!");
                    }
                    else {
                        std::string log = "[ACK] : Package (SEQ to : " + std::to_string(recvBuffer.seq) + ") sent successfully!";
                        logger.addLog(log);
                    }
                    // update send buffer
                    for(int i = baseSeq; i <= recvBuffer.ack; i++) {
                        std::lock_guard<std::mutex> lockGuard(bufferMutex);
                        sendBuffer.pop_front();
                    }
                    if(recvBuffer.isFIN()) {
                        return;
                    }
                    // update baseSeq
                    baseSeq = recvBuffer.ack + 1 > baseSeq ? recvBuffer.ack + 1 : baseSeq;
                    // check whether to reset timer
                    if(baseSeq == nextSeq) {
                        timer.stop();
                    }
                    else {
                        timer.start();
                    }
                }
                else {
                    logger.addLog("[LOG] Checksum invalid! Wait for timeout!");
                }
            }
            else {
                logger.addLog("[ERROR] : Package received from socket failed!");
            }
        }
    });
    recvThread.detach();
}

/**
 * begin to check timeout
 */
void beginTimeOut() {
    std::thread resendThread([&](){
        while(true) {
            while(!timer.isTimeout()) {}
            std::string log = "[TIMEOUT] : Package (SEQ from : " + std::to_string(baseSeq) + " to : " + std::to_string(nextSeq - 1) + ") re-send!";
            logger.addLog(log);
            int i = baseSeq;
            do {
                std::lock_guard<std::mutex> lockGuard(bufferMutex);
                struct Message message = sendBuffer[i-baseSeq];
                sendto(serverSocket, (char*) &message, sizeof(struct Message), 0, (struct sockaddr*) &clientAddress, sizeof(SOCKADDR));
                logger.addLog("[RE-SEND] : " + message2string(message));
            } while(++i < nextSeq);
        }
    });
    resendThread.detach();
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
            baseSeq
    };
    msgSYN.setSYN();
    msgSYN.setLen(0);
    msgSYN.setChecksum(&sendPseudoHeader);
    sendPackage(msgSYN);
    while(!sendBuffer.empty()) {
        // wait for send buffer to be flushed
    }
}

/**
 * send files in the specified directory
 */
void sendFiles() {
    // read files in "test/files" directory
    std::vector<FileDescriptor> files = getAllFiles(fileDir);
    for(auto file : files){
        std::string log = "[LOG] : Sending file: " + std::string(file.fileName) + " size: " + std::to_string(file.fileSize) + " bytes begin!";
        logger.addLog(log);
        // count the time
        auto start = std::chrono::steady_clock::now();
        sendFile(file);
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        log = "[LOG] : Send file: " + std::string(file.fileName) + " size: " + std::to_string(file.fileSize) + " bytes finish in " + std::to_string(duration) + " ms";
        logger.addLog(log);
    }
}

/**
 * send a file
 * @param file the destination of file
 */
void sendFile(struct FileDescriptor file) {
    // send the description of the file containing the file name and size
    struct Message msg{
            serverPort,
            clientPort,
            0,
            nextSeq
    };
    msg.setACK();
    msg.setFHD();
    msg.setLen(sizeof(struct FileDescriptor));
    msg.setData((char*) &file);
    msg.setChecksum(&sendPseudoHeader);
    sendPackage(msg);
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
                nextSeq
        };
        fileBuffer.setACK();
        fileBuffer.setLen(fmin(len,MSS));
        fileBuffer.setData(fileContent);
        fileBuffer.setChecksum(&sendPseudoHeader);
        len -= MSS;
        std::string log = "[Seg " + std::to_string(i) + " in " + std::to_string(segments) + "]";
        logger.addLog(log);
        while(sendBuffer.size() >= WINDOW_SIZE) {
            // wait for the space in send buffer
        }
        sendPackage(fileBuffer);
    }
}

/**
 * send a package to client
 * @param message message in package
 */
void sendPackage(struct Message message) {
    // add to the send buffer
    std::lock_guard<std::mutex> lockGuard(bufferMutex);
    sendBuffer.push_back(message);
    // send package
    if(!randomLoss()){
        sendto(serverSocket, (char*) &message, sizeof(struct Message), 0, (struct sockaddr*) &clientAddress, sizeof(SOCKADDR));
    }
    logger.addLog("[SEND] : " + message2string(message));
    // start timer
    if(baseSeq == nextSeq) {
        timer.start();
    }
    // update nextSeq
    nextSeq++;
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
            nextSeq
    };
    msgSYN.setFIN();
    msgSYN.setLen(0);
    msgSYN.setChecksum(&sendPseudoHeader);
    sendPackage(msgSYN);
    while(!sendBuffer.empty()) {
        // wait for send buffer to be flushed
    }
}