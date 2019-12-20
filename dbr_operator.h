#include <iostream>
#include <file_operator.h>
#include <defines.h>
#include <vector>
#include <map>
#include <string>
#include <QDebug>
#include <dbr.h>

#ifndef FAT32_H
#define FAT32_H

typedef struct {

} INVALID_FAT32_SYSTEM;

class DBROperator{

private:

    FileOperator * file_operator;

    std::string verify_str;
    std::vector<unsigned char> verify_num;

    typedef struct {
        unsigned begin_byte;
        unsigned size;
        const char * name;
    } read_sector;

    std::map<std::string, std::pair<unsigned, unsigned> > dbr_sectors;

    DBR dbr;

public:

    DBROperator():verify_str("FAT32   "),
        verify_num({0x55, 0xaa}){
        dbr_sectors["section size"] = std::pair<int, int>(0xb, 2);
        dbr_sectors["cluster section count"] = std::pair<int, int>(0xd, 1);
        dbr_sectors["reserved section count"] = std::pair<int, int>(0xe, 2);
        dbr_sectors["fat table count"] = std::pair<int, int>(0x10, 1);
        dbr_sectors["fat table section count"] = std::pair<int, int>(0x24, 4);
        dbr_sectors["root directory cluster"] = std::pair<int, int>(0x2c, 4);
        dbr_sectors["fat32 verification string"] = std::pair<int, int>(0x52, 8);
        dbr_sectors["fat32 verification number"] = std::pair<int, int>(0x1fe, 2);
    }

    void init_cluster_count(){
        unsigned file_size = file_operator->get_file_size();
        /* 真正开始存放簇的位置（2） */
        unsigned cluster_begin = (dbr.table_count * dbr.table_section_count + dbr.reserved_section_count) * dbr.section_size;
        /* 第0个簇的位置 */
        cluster_begin -= dbr.root_cluster * dbr.cluster_size * dbr.section_size;
        dbr.cluster_count = (file_size - cluster_begin) / (dbr.cluster_size * dbr.section_size);
    }

    void init_dbr(FileOperator * direct_reader){
        this->file_operator = direct_reader;
        dbr.section_size = file_operator->read_bytes(
                    dbr_sectors["section size"].first, dbr_sectors["section size"].second);
        dbr.table_count = file_operator->read_bytes(
                    dbr_sectors["fat table count"].first, dbr_sectors["fat table count"].second);
        dbr.table_section_count = file_operator->read_bytes(
                    dbr_sectors["fat table section count"].first, dbr_sectors["fat table section count"].second);
        dbr.reserved_section_count = file_operator->read_bytes(
                    dbr_sectors["reserved section count"].first, dbr_sectors["reserved section count"].second);
        dbr.cluster_size = file_operator->read_bytes(
                    dbr_sectors["cluster section count"].first, dbr_sectors["cluster section count"].second);
        dbr.root_cluster = file_operator->read_bytes(
                    dbr_sectors["root directory cluster"].first, dbr_sectors["root directory cluster"].second);
        init_cluster_count();
    }

    void init_dbr(unsigned section_size,
                  unsigned cluster_size,
                  unsigned all_size,
                  FileOperator * direct_writer){
        all_size = all_size * EVERY_MB * EVERY_KB;
        this->file_operator = direct_writer;
        file_operator->append_size(all_size);
        dbr.cluster_size = cluster_size;
        dbr.section_size = section_size;
        dbr.table_count = DEFAULT_TABLE_COUNT;

        dbr.root_cluster = DEFAULT_ROOT_CLUSTER;
        /* 浪费一点空间，保证后面的计算不会溢出 */
        dbr.reserved_section_count = dbr.cluster_size * dbr.root_cluster;

        /*
         * 解方程
         * reserved_section_count * section_size + 2 * L * section_size + (L * section_size / 4 - root_cluster) * cluster_size * section_size <= all_size
         * L是FAT所占section数，L取能够使得上式成立的最大值
         */
        unsigned L = (all_size - section_size * dbr.reserved_section_count + dbr.root_cluster * cluster_size * section_size)
                / (cluster_size * section_size * section_size / FAT_ITEM_SIZE + dbr.table_count * section_size);
        /*
         * 剩下的空间还可以放若干个簇，这时FAT会增加且只会增加2个扇区
         * cluster_remain = (remain - 2 * section_size) / cluster_size / section_size
         */
        unsigned remain = all_size - dbr.reserved_section_count * section_size - DEFAULT_TABLE_COUNT * section_size * L;
        remain -= (L * section_size / FAT_ITEM_SIZE - dbr.root_cluster) * section_size * cluster_size;
        remain -= dbr.table_count * section_size;

        dbr.cluster_count = section_size * L / FAT_ITEM_SIZE;
        if (remain > 0){
            unsigned remainCluster = remain / section_size / cluster_size;
            dbr.cluster_count += remainCluster;
            dbr.table_section_count = L + 1;
        }

        /* 写入DBR */
        file_operator->write_bytes(
                    dbr_sectors["section size"].first, dbr_sectors["section size"].second, dbr.section_size);
        file_operator->write_bytes(
                    dbr_sectors["fat table count"].first, dbr_sectors["fat table count"].second, dbr.table_count);
        file_operator->write_bytes(
                    dbr_sectors["fat table section count"].first, dbr_sectors["fat table section count"].second, dbr.table_section_count);
        file_operator->write_bytes(
                    dbr_sectors["reserved section count"].first, dbr_sectors["reserved section count"].second, dbr.reserved_section_count);
        file_operator->write_bytes(
                    dbr_sectors["cluster section count"].first, dbr_sectors["cluster section count"].second, dbr.cluster_size);
        file_operator->write_bytes(
                    dbr_sectors["root directory cluster"].first, dbr_sectors["root directory cluster"].second, dbr.root_cluster);
        file_operator->write_blocks(
                    dbr_sectors["fat32 verification string"].first, dbr_sectors["fat32 verification string"].second, verify_str.c_str());
        file_operator->write_blocks(
                    dbr_sectors["fat32 verification number"].first, dbr_sectors["fat32 verification number"].second, verify_num.data());

        /* 初始化FAT表 */
        unsigned fat_begin = dbr.reserved_section_count * section_size;
        char * buffer = new char[section_size];
        memset(buffer, 0, section_size);
        for (unsigned i = 0; i < dbr.table_section_count * dbr.table_count; ++i){
            file_operator->write_blocks(fat_begin + i * section_size, section_size, buffer);
        }
        for (unsigned i = 0; i < dbr.table_count; ++i){
            file_operator->write_bytes(fat_begin + i * dbr.table_section_count * dbr.section_size, sizeof(unsigned), FIRST_CLUSTER);
            file_operator->write_bytes(fat_begin + FAT_ITEM_SIZE + i * dbr.table_section_count * dbr.section_size, sizeof(unsigned), SECOND_CLUSTER);
            file_operator->write_bytes(fat_begin + FAT_ITEM_SIZE * dbr.root_cluster + i * dbr.table_section_count * dbr.section_size, sizeof(unsigned), INVALID_FILE_CLUSTER);
        }
        delete []buffer;

        /* 清除根簇 */
        unsigned cluster_begin = fat_begin + dbr.table_count * dbr.table_section_count * dbr.section_size;
        buffer = new char[dbr.cluster_size * dbr.section_size];
        memset(buffer, 0, dbr.cluster_size * dbr.section_size);
        file_operator->write_blocks(cluster_begin, dbr.root_cluster * dbr.section_size, buffer);
        delete []buffer;

        //qDebug() << "success";
    }

    void verify_fat32() {
        /*
         * 检查是否为正确的FAT32文件系统
         */
        for (unsigned i = 0; i < verify_str.size(); ++i) {
            char temp_num = file_operator->read_bytes(dbr_sectors["fat32 verification string"].first + i, 1);
            if (verify_str[i] != temp_num)
                throw INVALID_FAT32_SYSTEM{};
        }
        for (unsigned i = 0; i < verify_num.size(); ++i) {
            unsigned char temp_num = file_operator->read_bytes(dbr_sectors["fat32 verification number"].first + i, 1);
            if (verify_num[i] != temp_num)
                throw INVALID_FAT32_SYSTEM{};
        }
    }

    DBR get_dbr(){
        return dbr;
    }

    FileOperator * get_file_operator(){
        return file_operator;
    }

    unsigned get_next_cluster(unsigned cluster) {
        unsigned begin = dbr.reserved_section_count * dbr.section_size;
        unsigned result = file_operator->read_bytes(begin + cluster * FAT_ITEM_SIZE, FAT_ITEM_SIZE);
        return result;
    }

    void delete_cluster(unsigned cluster){
        unsigned begin = dbr.reserved_section_count * dbr.section_size;
        file_operator->write_bytes(begin + cluster * FAT_ITEM_SIZE, FAT_ITEM_SIZE, INVALID_FILE_CLUSTER);
    }

};

#endif
