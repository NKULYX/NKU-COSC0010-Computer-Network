#include <stdio.h>
#include <winsock.h>
#include <stdbool.h>
#include <time.h>

#pragma comment(lib, "Ws2_32.lib")

#define MESS_MAX_LEN 100


struct IP {
    unsigned char ip_1;
    unsigned char ip_2;
    unsigned char ip_3;
    unsigned char ip_4;
    unsigned short port;
}myIP, destIP;

struct Message {
    time_t time;
    struct IP fromIP;
    struct IP toIP;
    char message[MESS_MAX_LEN];
};

SOCKET mySocket;
SOCKET destSocket;
SOCKADDR_IN myAddr;
SOCKADDR_IN destAddr;

bool sendProc();
bool checkQuit(char *buff);
_Noreturn DWORD WINAPI receiveProc(LPVOID lparam);
void printMessage(struct Message);

int main()
{
    WSADATA wsaData;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed!\n");
    }
    else {
        printf("WSAStartup success!\n");
    }

    // create socket
    mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mySocket == INVALID_SOCKET) {
        printf("Create socket failed! Failure code: %d\n", WSAGetLastError());
    }
    else {
        printf("Create socket success!\n");
    }

    printf("Please input your IPv4 address:");
    scanf_s("%d.%d.%d.%d",&myIP.ip_1,&myIP.ip_2,&myIP.ip_3,&myIP.ip_4);

    printf("Please input your port: ");
    scanf_s("%d", &myIP.port);

    myAddr.sin_family = AF_INET;
    myAddr.sin_port = (myIP.port);
    myAddr.sin_addr.S_un.S_un_b.s_b1 = (myIP.ip_1);
    myAddr.sin_addr.S_un.S_un_b.s_b2 = (myIP.ip_2);
    myAddr.sin_addr.S_un.S_un_b.s_b3 = (myIP.ip_3);
    myAddr.sin_addr.S_un.S_un_b.s_b4 = (myIP.ip_4);

    // bind
    if (bind(mySocket, (SOCKADDR*)&myAddr, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        printf("Bind failed! Failure code: %d\n", WSAGetLastError());
    }
    else {
        printf("Bind success!\n");
    }

    if(listen(mySocket, 5) < 0){
        printf("Listen failed!\n");
    }
    else {
        printf("Listen success!\n");
    }

    HANDLE recThreadHandle = CreateThread(NULL, (SIZE_T) NULL, receiveProc, (LPVOID) mySocket, 0, 0);

    printf("Please input destination IPv4 address:");
    scanf_s("%d.%d.%d.%d",&destIP.ip_1,&destIP.ip_2,&destIP.ip_3,&destIP.ip_4);

    printf("Please input your destination port: ");
    scanf_s("%d", &destIP.port);

    // initialize destination socket information
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = (destIP.port);
    destAddr.sin_addr.S_un.S_un_b.s_b1 = (destIP.ip_1);
    destAddr.sin_addr.S_un.S_un_b.s_b2 = (destIP.ip_2);
    destAddr.sin_addr.S_un.S_un_b.s_b3 = (destIP.ip_3);
    destAddr.sin_addr.S_un.S_un_b.s_b4 = (destIP.ip_4);

    while(sendProc());

    closesocket(mySocket);
    CloseHandle(recThreadHandle);
    WSACleanup();
}

bool sendProc()
{
    printf("=========================== Send ===========================\n");
    struct Message msg;
    scanf("%s", &(msg.message));
    if(checkQuit(msg.message)){
        return false;
    }
    destSocket = socket(AF_INET, SOCK_STREAM,0);
    if (destSocket == INVALID_SOCKET) {
        printf("Socket creation fails! Failure code: %d\n", WSAGetLastError());
    }
    else {
        printf("Socket creation successes!\n");
    }
    if (connect(destSocket, (SOCKADDR*)&destAddr, sizeof(destAddr)) == SOCKET_ERROR) {
        printf("Connection to %d.%d.%d.%d:%d fails!\n",destIP.ip_1,destIP.ip_2,destIP.ip_3,destIP.ip_4,destIP.port);
    }
    else {
        printf("Connection to %d.%d.%d.%d:%d successes!\n",destIP.ip_1,destIP.ip_2,destIP.ip_3,destIP.ip_4,destIP.port);
        time(&(msg.time));
        msg.fromIP = myIP;
        msg.toIP = destIP;
        int sendLen = send(destSocket, (char*)&msg, sizeof(struct Message), 0);
        if(sendLen == 0) {
            printf("Send fails!\n");
        } else {
            printf("Send to %d.%d.%d.%d:%d message: %s\n",destIP.ip_1,destIP.ip_2,destIP.ip_3,destIP.ip_4,destIP.port, msg.message);
        }
    }
    closesocket(destSocket);
    printf("Socket closed!\n");
    printf("=========================== Send ===========================\n\n");
    return true;
}

bool checkQuit(char *buff)
{
    char * target = "QUIT!";
    char * valid = strstr(buff, target);
    if(valid == NULL){
        return false;
    }
    else {
        return true;
    }
}

_Noreturn DWORD WINAPI receiveProc(LPVOID lparam)
{
    struct Message msg;
    SOCKET sock = (SOCKET) (LPVOID) lparam;
    SOCKADDR_IN dsetAddr;
    int destAddrLen = sizeof(SOCKADDR_IN);
    while(1) {
        SOCKET socketConn = accept(sock, (SOCKADDR *)&dsetAddr, &destAddrLen);
        int recLen = recv(socketConn, (char *)&msg, sizeof(struct Message), 0);
        if(recLen < 0){
            printf("Receive failed!\n");
        }
        else {
            printMessage(msg);
        }
        closesocket(socketConn);
    }
}

void printMessage(struct Message message)
{
    printf("=========================== Recv ===========================\n");
    char timeBuff[50];
    ctime_s(timeBuff, 50, &message.time);
    printf("From IPv4 address : %d.%d.%d.%d:%d\n",message.fromIP.ip_1,message.fromIP.ip_2,message.fromIP.ip_3,message.fromIP.ip_4,message.fromIP.port);
    printf("Send time : %s", timeBuff);
    time_t recTime;
    time(&recTime);
    ctime_s(timeBuff, 50, &recTime);
    printf("Recv time : %s", timeBuff);
    printf("Message : %s\n", message.message);
    printf("=========================== Recv ===========================\n\n");
    printf("=========================== Send ===========================\n");
}
