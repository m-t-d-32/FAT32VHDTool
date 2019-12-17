#include <stdio.h>
#include "Exception.h"
#include <string.h>
#include <stdlib.h>
#ifndef DirectRead_H
#define DirectRead_H

extern INVALID_DRIVE idr;
extern INVALID_READ rdr;

class DirectRead {
private:
    FILE * devFile;
public:
    DirectRead(const char * filename){
        devFile = fopen(filename, "rb");
        if (!devFile){
            throw idr;
        }
    }
    /*
     * 读取begin开始的size个字节（不超过4字节）并返回
     */
    unsigned read_bytes(long begin, unsigned size){
        if (size > sizeof(unsigned)){
            throw rdr;
        }
        unsigned result = 0;
        int errcode = fseek(devFile, begin, SEEK_SET);
        if (errcode){
            throw idr;
        }
        fread((void *)(&result), size, 1, devFile);
        return result;
    }
    /*
     * 将若干个字节读入缓存
     */
    void read_blocks(long begin, unsigned size, void * buffer){
        int errcode = fseek(devFile, begin, SEEK_SET);
        if (errcode){
            throw idr;
        }
        fread(buffer, size, 1, devFile);
    }
    ~DirectRead(){
        fclose(devFile);
        devFile = NULL;
    }
};

#endif
