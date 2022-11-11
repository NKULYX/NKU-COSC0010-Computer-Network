//
// Created by Lenovo on 2022/11/11.
//

#include <iostream>
#include <Util.h>
int main()
{
    std::vector<FileDescriptor> files = getAllFiles("../test/files");
    for(auto file : files) {
        std::cout << file.fileName << " " << file.fileSize << std::endl;
    }
}
