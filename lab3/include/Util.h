//
// Created by Lenovo on 2022/11/11.
//

#ifndef LAB3_UTIL_H
#define LAB3_UTIL_H

#include <iostream>
#include <string>
#include <vector>
#include <winsock.h>

const int MSS = 1024;
const int LOSS_RATE = 1;

struct FileDescriptor {
    char fileName[20];
    unsigned int fileSize;
};

std::vector<FileDescriptor> getAllFiles(const std::string& directory) {
    std::vector<FileDescriptor> files;
    WIN32_FIND_DATAA FindFileData;
    HANDLE hFind = FindFirstFileA((directory + "\\*").c_str(), &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "Invalid file handle. Error is " << GetLastError() << std::endl;
        return files;
    }
    do {
        if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        FileDescriptor file{};
        strcpy(file.fileName, FindFileData.cFileName);
        file.fileSize = FindFileData.nFileSizeLow;
        files.push_back(file);
    } while (FindNextFileA(hFind, &FindFileData) != 0);
    FindClose(hFind);
    return files;
}

struct Message {
    unsigned short sourcePort{};
    unsigned short destinationPort{};
    int ack{};
    int seq{};
    unsigned short flagAndLength = 0;
    unsigned short checksum{};
    char data[MSS]{};
    void setLen(short int len){
        flagAndLength |= (len & 0x07FF);
    }
    unsigned short getLen() const {
        return flagAndLength & 0x07FF;
    }
    void setACK() {
        flagAndLength |= 0x1000;
    };
    bool isACK() const {
        return flagAndLength & 0x1000;
    };
    void setSYN() {
        flagAndLength |= 0x2000;
    };
    bool isSYN() const {
        return flagAndLength & 0x2000;
    };
    void setFIN() {
        flagAndLength |= 0x4000;
    };
    bool isFIN() const {
        return flagAndLength & 0x4000;
    };
    void setRep() {
        flagAndLength |= 0x8000;
    };
    bool isRep() const {
        return flagAndLength & 0x8000;
    };
    void setFHD() {
        flagAndLength |= 0x0800;
    };
    bool isFHD() const {
        return flagAndLength & 0x0800;
    };
    void setData(char* data);
    void setChecksum(struct PseudoHeader *pseudoHeader);
    bool checksumValid(struct PseudoHeader *pseudoHeader);
};

struct PseudoHeader {
    unsigned long sourceIP{};
    unsigned long destinationIP{};
    char zero = 0;
    char protocol = 6;
    short int length = sizeof(struct Message);
};

void Message::setData(char* sourceData) {
    memset(data, 0, MSS);
    memcpy(data, sourceData, getLen());
}

void Message::setChecksum(struct PseudoHeader *pseudoHeader) {
    this->checksum = 0;
    unsigned long long int sum = 0;
    for (int i = 0; i < sizeof(struct PseudoHeader) / 2; i++) {
        sum += ((unsigned short int *) pseudoHeader)[i];
    }
    for (int i = 0; i < sizeof(struct Message) / 2; i++) {
        sum += ((unsigned short int *) this)[i];
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    this->checksum = ~sum;
}

bool Message::checksumValid(struct PseudoHeader *pseudoHeader){
    unsigned long long int sum = 0;
    for (int i = 0; i < sizeof(struct PseudoHeader) / 2; i++) {
        sum += ((unsigned short int *) pseudoHeader)[i];
    }
    for (int i = 0; i < sizeof(struct Message) / 2; i++) {
        sum += ((unsigned short int *) this)[i];
    }
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    return sum == 0x000ffff;
}

void printMessage(struct Message message) {
    std::cout << "{ Package "
            << "[SYN:" << message.isSYN() << "] "
            << "[ACK:" << message.isACK() << "] "
            << "[FIN:" << message.isFIN() << "] "
            << "[ack:" << message.ack << "] "
            << "[seq:" << message.seq << "] "
            << "[Len:" << message.getLen() << "] }"
            << std::endl;
}

bool randomLoss() {
    return rand() % 100 < LOSS_RATE;
}

#endif //LAB3_UTIL_H
