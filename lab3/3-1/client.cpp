//
// Created by Lenovo on 2022/11/11.
//


#include <Util.h>
#include <thread>
#include <fstream>

#pragma comment(lib, "Ws2_32.lib")

SOCKET clientSocket;
SOCKADDR_IN clientAddress;
std::string clientIP = "127.0.0.1";
unsigned short clientPort = 8088;
SOCKADDR_IN serverAddress;
std::string serverIP = "127.0.0.1";
unsigned short serverPort = 8888;

struct PseudoHeader sendPseudoHeader{};
struct PseudoHeader recvPseudoHeader{};
const int RTO = 500;
bool ACK_FLAG = false;
int CURRENT_SEQ = 0;

std::string fileDir = "../test/3-1";

bool initConnection();
bool establishConnection();
void sendPackage(struct Message message);
void sendACK(int ackNum);

[[noreturn]] void beginRecv();

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

    // establish connection via three times handshake
//    while(!establishConnection()) {
//        std::cout << "Establishment failed! Retry in 1 seconds..." << std::endl;
//        Sleep(1000);
//    }

    beginRecv();

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
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Create socket failed! Failure code: " << WSAGetLastError() << std::endl;
        return false;
    }
    else {
        std::cout << "Create socket success!" << std::endl;
    }

    // bind
    if (bind(clientSocket, (SOCKADDR*)&clientAddress, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        std::cout << "Bind failed! Failure code: " << WSAGetLastError() << std::endl;
        return false;
    }
    else {
        std::cout << "Bind success!" << std::endl;
    }

    // initialize pseudo header
    sendPseudoHeader.sourceIP = clientAddress.sin_addr.S_un.S_addr;
    sendPseudoHeader.destinationIP = serverAddress.sin_addr.S_un.S_addr;
    recvPseudoHeader.sourceIP = serverAddress.sin_addr.S_un.S_addr;
    recvPseudoHeader.destinationIP = clientAddress.sin_addr.S_un.S_addr;

    return true;
}

bool establishConnection() {

}



[[noreturn]] void beginRecv() {
    // receive message from server
    struct Message recvBuffer{};
    // write to file
    std::fstream file;
    unsigned int fileSize;
    unsigned int currentSize = 0;
    std::string filename;
    while(true) {
        int serverAddressLength = sizeof(SOCKADDR);
        int recvLength = recvfrom(clientSocket, (char*) &recvBuffer, sizeof(struct Message), 0, (struct sockaddr *) &serverAddress, &serverAddressLength);
        if(recvLength > 0) {
            printMessage(recvBuffer);
            if(recvBuffer.checksumValid(&recvPseudoHeader) && recvBuffer.seq == CURRENT_SEQ) {
                // if message contains file header
                if(recvBuffer.isFHD()) {
                    struct FileDescriptor fileDescriptor{};
                    memcpy(&fileDescriptor, recvBuffer.data, sizeof(struct FileDescriptor));
                    std::cout << "Receive file header: [Name:" << fileDescriptor.fileName << "] [Size:" << fileDescriptor.fileSize << "]" << std::endl;
                    fileSize = fileDescriptor.fileSize;
                    filename = fileDir + "/" + fileDescriptor.fileName;
                    currentSize = 0;
                    // create file
                    file.open(filename, std::ios::out | std::ios::binary);
                }
                else {
                    // write to file
                    file.write(recvBuffer.data, recvBuffer.getLen());
                    currentSize += recvBuffer.getLen();
                    if(currentSize >= fileSize){
                        std::cout << "File receive success!" << filename << std::endl;
                        file.close();
                    }
                }
                // send ACK to server
                if(!randomLoss()){
                    sendACK(recvBuffer.seq);
                }
                // update current seq
                CURRENT_SEQ = (CURRENT_SEQ + 1) % 2;
            }
            // if package is not valid or receive seq is not equal to current seq
            // need to send last ACK again
            else {
                if(!randomLoss()) {
                    sendACK((CURRENT_SEQ + 1) % 2);
                }
            }
        }
    }
}

void sendACK(int ackNum) {
    // send ACK to server
    struct Message sendBuffer{
            clientPort,
            serverPort,
            ackNum,
            CURRENT_SEQ
    };
    sendBuffer.setACK();
    sendBuffer.setLen(0);
    sendBuffer.setChecksum(&sendPseudoHeader);
    sendto(clientSocket, (char*) &sendBuffer, sizeof(struct Message), 0, (struct sockaddr *) &serverAddress, sizeof(SOCKADDR));
}