#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef DirectRead_H
#define DirectRead_H

#include <QFile>
#include <QString>
#include <QFileInfo>
#include <QDebug>

typedef struct {

} INVALID_SRC_FILE;

class FileOperator {
private:
    QFile * devfile;
public:
    FileOperator(QString filename){
        devfile = new QFile(filename);
        if (!devfile->open(QIODevice::ReadWrite)){
            devfile = NULL;
            throw INVALID_SRC_FILE{};
        }
    }

    /*
     * 读取begin开始的size个字节（不超过4字节）并返回
     */
    unsigned read_bytes(long begin, unsigned size){
        unsigned result = 0;
        devfile->seek(begin);
        devfile->read((char *)(&result), size);
        return result;
    }
    /*
     * 写入begin开始的size个字节（不超过4字节）
     */
    void write_bytes(long begin, unsigned size, unsigned data){
        devfile->seek(begin);
        devfile->write((char *)(&data), size);
    }
    /*
     * 写入若干字节
     */
    void write_blocks(long begin, unsigned size, const void * buffer){
        devfile->seek(begin);
        devfile->write((char *)buffer, size);
    }
    /*
     * 将若干个字节读入缓存
     */
    void read_blocks(long begin, unsigned size, void * buffer){
        devfile->seek(begin);
        devfile->read((char *)buffer, size);
    }
    /*
     * 获取文件大小
     */
    long get_file_size(){
        return QFileInfo(*devfile).size();
    }

    void append_size(long l){
        devfile->seek(l - 1);
        devfile->write("w", 1);
        devfile->seek(0);
    }

    ~FileOperator(){
        devfile->close();
        delete devfile;
        devfile = NULL;
    }
};

#endif
