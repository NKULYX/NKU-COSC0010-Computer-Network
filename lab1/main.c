#include <stdio.h>
#include <winsock.h>
#include <stdbool.h>

#pragma comment(lib, "Ws2_32.lib")

SOCKET mySocket;
SOCKET destSocket;
unsigned short myPort;
SOCKADDR_IN myAddr;
unsigned short destPort;
SOCKADDR_IN destAddr;

_Noreturn DWORD WINAPI threadProc(LPVOID lparam);
bool checkQuit(char *buff);


int main()
{
    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed!\n");
    } else {
        printf("WSAStartup success!\n");
    }

    // 建立socket
    mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mySocket == INVALID_SOCKET) {
        printf("Create socket failed! Failure code: %d\n", WSAGetLastError());
    } else {
        printf("Create socket success!\n");
    }

    printf("Please input your port: ");
    scanf_s("%d", &myPort);

    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(myPort);
    myAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    // 绑定
    if (bind(mySocket, (SOCKADDR*)&myAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        printf("Bind failed! Failure code: %d\n", WSAGetLastError());
    } else {
        printf("Bind success!\n");
    }

    if(listen(mySocket, 5) < 0){
        printf("Listen failed!\n");
    } else {
        printf("Listen success!\n");
    }

    HANDLE recvThreadHandle = CreateThread(NULL, (SIZE_T) NULL, threadProc, (LPVOID)mySocket, 0, 0);

    printf("Please input your destination port: ");
    scanf_s("%d", &destPort);

    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(destPort);
    destAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

    while(1) {
        destSocket = socket(AF_INET, SOCK_STREAM,0);
        if (destSocket == INVALID_SOCKET) {
            printf("Create socket failed! Failure code: %d\n", WSAGetLastError());
        } else {
            printf("Create socket success!\n");
        }
        char sendBuff[50];
        printf("Message:");
        scanf("%s", &sendBuff);
        if(checkQuit(sendBuff)){
            break;
        }
        if (connect(destSocket, (SOCKADDR*)&destAddr, sizeof(destAddr)) == SOCKET_ERROR) {
            printf("Connecting to destSocket fails!\n");
        } else {
            printf("Connecting to destSocket successes!\n");
            int sendLen = send(destSocket, sendBuff, 50, 0);
            if(sendLen == 0) {
                printf("Send fails!\n");
            } else {
                printf("Send to port: %d message: %s\n", destPort, sendBuff);
            }
        }
        closesocket(destSocket);
    }

    closesocket(mySocket);
    CloseHandle(recvThreadHandle);
    WSACleanup();
}

bool checkQuit(char *buff) {
    char * target = "QUIT!";
    char * valid = strstr(buff, target);
    if(valid == NULL){
        return false;
    } else {
        return true;
    }
}

_Noreturn DWORD WINAPI threadProc(LPVOID lparam)
{
    char recvBuff[50];
    SOCKET sock = (SOCKET) (LPVOID) lparam;
    SOCKADDR_IN dsetAddr;
    int destAddrLen = sizeof(SOCKADDR_IN);
    while(1) {
        SOCKET socketConn = accept(sock, (SOCKADDR *)&dsetAddr, &destAddrLen);
        int recLen = recv(socketConn,recvBuff, 50,0);
        if(recLen < 0){
            printf("Receive failed!\n");
        } else {
            printf("Receive: %s\n", recvBuff);
        }
    }
}
