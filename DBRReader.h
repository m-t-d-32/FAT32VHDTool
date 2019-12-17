#include <iostream>
#include "DirectRead.h"
#include <vector>
#include <map>
#include <string>

#ifndef FAT32_H
#define FAT32_H
#define FAT_ITEM_SIZE 4


typedef struct {
    unsigned table_count;   /* FAT表个数 */
    unsigned section_size;  /* 每扇区字节数 */
    unsigned table_section_count; /* FAT表扇区数 */
    unsigned reserved_section_count; /* 保留扇区数 */
    unsigned cluster_size; /* 每簇扇区数 */
    unsigned root_cluster; /* 根目录簇序号 */
} DBR;

class DBRReader{

private:

    DirectRead * directReader;

    typedef struct {
        unsigned begin_byte;
        unsigned size;
        const char * name;
    } read_sector;

    std::map<std::string, std::pair<unsigned, unsigned> > dbr_sectors;

    DBR dbr;

public:

    DBRReader(DirectRead * directReader){
        dbr_sectors["section size"] = std::pair<int, int>(0xb, 2);
        dbr_sectors["cluster section count"] = std::pair<int, int>(0xd, 1);
        dbr_sectors["reserved section count"] = std::pair<int, int>(0xe, 2);
        dbr_sectors["fat table count"] = std::pair<int, int>(0x10, 1);
        dbr_sectors["fat table section count"] = std::pair<int, int>(0x24, 4);
        dbr_sectors["root directory cluster"] = std::pair<int, int>(0x2c, 4);
        dbr_sectors["fat32 verification string"] = std::pair<int, int>(0x52, 8);
        dbr_sectors["fat32 verification number"] = std::pair<int, int>(0x1fe, 2);

        this->directReader = directReader;
        init_dbr();
    }

    void init_dbr(){
        dbr.section_size = directReader->read_bytes(
                    dbr_sectors["section size"].first, dbr_sectors["section size"].second);
        dbr.table_count = directReader->read_bytes(
                    dbr_sectors["fat table count"].first, dbr_sectors["fat table count"].second);
        dbr.table_section_count = directReader->read_bytes(
                    dbr_sectors["fat table section count"].first, dbr_sectors["fat table section count"].second);
        dbr.reserved_section_count = directReader->read_bytes(
                    dbr_sectors["reserved section count"].first, dbr_sectors["reserved section count"].second);
        dbr.cluster_size = directReader->read_bytes(
                    dbr_sectors["cluster section count"].first, dbr_sectors["cluster section count"].second);
        dbr.root_cluster = directReader->read_bytes(
                    dbr_sectors["root directory cluster"].first, dbr_sectors["root directory cluster"].second);
    }

    void verify_fat32() {
        /*
         * 检查是否为正确的FAT32文件系统
         */
        char verify_str[] = "FAT32   ";
        unsigned char verify_num[] = {0x55, 0xaa};
        for (unsigned i = 0; i < sizeof(verify_str) - 1; ++i) {
            char temp_num = directReader->read_bytes(dbr_sectors["fat32 verification string"].first + i, 1);
            if (verify_str[i] != temp_num)
                throw rdr;
        }
        for (unsigned i = 0; i < sizeof(verify_num); ++i) {
            unsigned char temp_num = directReader->read_bytes(dbr_sectors["fat32 verification number"].first + i, 1);
            if (verify_num[i] != temp_num)
                throw rdr;
        }
    }

    DBR get_dbr(){
        return dbr;
    }

    DirectRead * getDirectReader(){
        return directReader;
    }

    unsigned get_next_cluster(unsigned cluster) {
        unsigned begin = dbr.reserved_section_count * dbr.section_size;
        unsigned result = directReader->read_bytes(begin + cluster * FAT_ITEM_SIZE, FAT_ITEM_SIZE);
        return result;
    }

};

#endif
