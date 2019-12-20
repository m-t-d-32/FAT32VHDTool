#ifndef FAT32_FILE_H
#define FAT32_FILE_H

#include <string>
#include <QString>

struct FAT32_file{
    char file_name[9];   /*文件名*/
    char extra_name[4];  /*扩展名*/
    unsigned char file_type;    /*文件类型*/
    unsigned cluster;   /*文件起始簇号*/
    unsigned size;  /*文件大小（字节数）*/
    QString long_filename; /*长文件名*/

    unsigned create_date;   /*创建日期 */
    unsigned create_time;   /*创建时间*/
    unsigned create_mms; /*创建毫秒数*/
    unsigned modified_date;    /*上次读取日期*/
    unsigned modified_time;    /*上次读取时间*/

    FAT32_file(unsigned char file_type, unsigned cluster,
               unsigned size, QString long_filename){
        this->file_type = file_type;
        this->cluster = cluster;
        this->size = size;
        this->long_filename = long_filename;
    }

    FAT32_file(){}
};

#endif // FAT32_FILE_H
