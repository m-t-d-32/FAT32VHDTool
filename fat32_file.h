#ifndef FAT32_FILE_H
#define FAT32_FILE_H

#include <string>
#include <QString>

typedef struct {
    char file_name[9];   /*文件名*/
    char extra_name[4];  /*扩展名*/
    unsigned char file_type;    /*文件类型*/
    unsigned create_time;   /*创建时间*/
    unsigned last_read_time;    /*上次读取时间*/
    unsigned cluster;   /*文件起始簇号*/
    unsigned size;  /*文件大小（字节数）*/
    QString long_filename; /*长文件名*/
}FAT32_file;

#endif // FAT32_FILE_H
