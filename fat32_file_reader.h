#include <file_operator.h>
#include <dbr_operator.h>
#include <string>
#include <iostream>
#include <vector>
#include <QDebug>
#include <defines.h>
#include <QTextCodec>

#ifndef FAT32_file_H
#define FAT32_file_H

class FAT32FileReader{

private:

    FileOperator * file_operator;

    DBR dbr_info;

    typedef struct {
        unsigned begin_byte;
        unsigned size;
        const char * name;
    } read_sector;

    std::map<std::string, std::pair<unsigned, unsigned> > file_sectors;

public:

    FAT32FileReader(FileOperator * direct_reader, DBR dbr_info){
        file_sectors["file name"] = std::pair<int, int>(0x00, 8);
        file_sectors["extra name"] = std::pair<int, int>(0x08, 3);
        file_sectors["file type"] = std::pair<int, int>(0x0b, 1);
        file_sectors["high cluster"] = std::pair<int, int>(0x14, 2);
        file_sectors["low cluster"] = std::pair<int, int>(0x1a, 2);
        file_sectors["file size"] = std::pair<int, int>(0x1c, 4);

        file_sectors["create date"] = std::pair<int, int>(0x10, 2);
        file_sectors["create time"] = std::pair<int, int>(0x0e, 2);
        file_sectors["create mms"] = std::pair<int, int>(0x0d, 1);
        file_sectors["modified date"] = std::pair<int, int>(0x18, 2);
        file_sectors["modified time"] = std::pair<int, int>(0x16, 2);

        this->file_operator = direct_reader;
        this->dbr_info = dbr_info;
    }

    auto get_file_sectors(){
        return file_sectors;
    }

    std::vector<FAT32_file> get_valid_files(const std::vector<unsigned> clusters) {

        std::vector<FAT32_file> results;

        unsigned clusters_index = 0;
        unsigned cluster_item_index = 0;
        bool parsing_long = false;

        while(true){
            if (!parsing_long){
                FAT32_file result;
                unsigned begin = (dbr_info.cluster_size * (clusters[clusters_index] - dbr_info.root_cluster)
                      + dbr_info.reserved_section_count + dbr_info.table_count * dbr_info.table_section_count) * dbr_info.section_size;
                begin += cluster_item_index * EVERY_ITEM_LENGTH;
                std::pair<int, int> filenameinfo = file_sectors["file name"];
                unsigned i = 0;
                for (i = 0; i < filenameinfo.second; ++i){
                    result.file_name[i] = file_operator->read_bytes(filenameinfo.first + begin + i, 1);
                    if (!result.file_name[i] || result.file_name[i] == ' ') break;
                }
                for (; i < sizeof(result.file_name); ++i){
                    result.file_name[i] = 0;
                }
                std::pair<int, int> extranameinfo = file_sectors["extra name"];
                for (i = 0; i < extranameinfo.second; ++i){
                    result.extra_name[i] = file_operator->read_bytes(extranameinfo.first + begin + i, 1);
                    if (result.extra_name[i] == ' ') break;
                }
                for (; i < sizeof(result.extra_name); ++i){
                    result.extra_name[i] = 0;
                }

                result.create_date = file_operator->read_bytes(file_sectors["create date"].first + begin, file_sectors["create date"].second);
                result.create_time = file_operator->read_bytes(file_sectors["create time"].first + begin, file_sectors["create time"].second);
                result.create_mms = file_operator->read_bytes(file_sectors["create mms"].first + begin, file_sectors["create mms"].second);
                result.modified_date = file_operator->read_bytes(file_sectors["modified date"].first + begin, file_sectors["modified date"].second);
                result.modified_time = file_operator->read_bytes(file_sectors["modified time"].first + begin, file_sectors["modified time"].second);

                result.file_type = file_operator->read_bytes(file_sectors["file type"].first + begin, file_sectors["file type"].second);
                result.size = file_operator->read_bytes(file_sectors["file size"].first + begin, file_sectors["file size"].second);
                result.cluster = 0;
                result.cluster = file_operator->read_bytes(file_sectors["high cluster"].first + begin, file_sectors["high cluster"].second);
                result.cluster <<= file_sectors["high cluster"].second * 8;
                result.cluster |= file_operator->read_bytes(file_sectors["low cluster"].first + begin, file_sectors["low cluster"].second);
                rstrip(result.file_name);
                rstrip(result.extra_name);

                if (!result.file_name[0]){
                    break;
                }
                else if (!strcmp(result.file_name, "..") ||
                         !strcmp(result.file_name, ".") ||
                         is_deleted(result.file_name[0]) ||
                         is_long(result) ||
                         is_volume(result)){
                    if (!add_1(clusters_index, cluster_item_index, clusters, dbr_info)){
                        break;
                    }
                }
                else {
                    results.push_back(result);
                    parsing_long = true;
                    continue;
                }
            }
            else {
                /*修改最后一个项*/
                FAT32_file result = results.back();
                results.pop_back();

                unsigned bk_clusters_index = clusters_index;
                unsigned bk_cluster_item_index = cluster_item_index;
                std::vector<unsigned short> temp_long_filename;
                while (true){
                    /*每次向前一个目录项*/                    
                    if (!sub_1(bk_clusters_index, bk_cluster_item_index, dbr_info)){
                        break;
                    }

                    unsigned begin = (dbr_info.cluster_size * (clusters[bk_clusters_index] - dbr_info.root_cluster)
                          + dbr_info.reserved_section_count + dbr_info.table_count * dbr_info.table_section_count) * dbr_info.section_size;
                    begin += bk_cluster_item_index * EVERY_ITEM_LENGTH;

                    unsigned long_begin[] = { 0x1, 0xe, 0x1c }, long_end[] = { 0xb, 0x1a, 0x20 };

                    unsigned char ibyte = (unsigned char)file_operator->read_bytes(begin, sizeof(char));
                    unsigned char xbyte = (unsigned char)file_operator->read_bytes(file_sectors["file type"].first + begin, file_sectors["file type"].second);
                    if (is_deleted(ibyte)){
                        /*如果是删除项一定不是长目录项*/
                        break;
                    }
                    else if (!is_long(xbyte)){
                        /*如果属性位不是0x0f一定不是长目录项*/
                        break;
                    }

                    bool end = false;
                    for (unsigned i = 0; i < sizeof(long_begin) / sizeof(unsigned); ++i){
                        for (unsigned j = long_begin[i]; j < long_end[i]; j += sizeof(unsigned short)){
                            unsigned short temp_char = file_operator->read_bytes(j + begin, sizeof(unsigned short));
                            if (is_long_end(temp_char)){
                                /*0xff表示是最后一个目录项*/
                                end = true;
                                break;
                            }
                            temp_long_filename.push_back(temp_char);
                        }
                        if (end) break;
                    }

                    if (is_last_long(file_operator->read_bytes(begin, sizeof(char)))){
                        /*0x0位的一个字节第6个二进制位是1，表示是最后一个目录项*/
                        break;
                    }
                    else if (end){
                        break;
                    }
                }

                if (temp_long_filename.size() <= 0) {
                    for (unsigned i = 0; result.file_name[i]; ++i)
                        result.long_filename += tolower(char(result.file_name[i]));
                    if (result.extra_name[0]){
                        result.long_filename += ".";
                        for (unsigned i = 0; result.extra_name[i]; ++i)
                            result.long_filename += tolower(char(result.extra_name[i]));
                    }
                }

                else {
                    temp_long_filename.push_back(0);
                    result.long_filename = QString::fromUtf16(temp_long_filename.data());
                    //qDebug() << result.long_filename;
                }
                /*修改后返回*/
                results.push_back(result);
                parsing_long = false;

                /*修改指针*/
                if (!add_1(clusters_index, cluster_item_index, clusters, dbr_info)){
                    break;
                }
            }
        }
        return results;
    }

    void delete_file(const std::vector<unsigned> clusters, FAT32_file file) {

        std::vector<FAT32_file> results;

        unsigned clusters_index = 0;
        unsigned cluster_item_index = 0;

        FAT32_file result;
        while (true){
            unsigned begin = (dbr_info.cluster_size * (clusters[clusters_index] - dbr_info.root_cluster)
                  + dbr_info.reserved_section_count + dbr_info.table_count * dbr_info.table_section_count) * dbr_info.section_size;
            begin += cluster_item_index * EVERY_ITEM_LENGTH;

            std::pair<int, int> filenameinfo = file_sectors["file name"];
            unsigned i = 0;
            result.file_name[0] = file_operator->read_bytes(filenameinfo.first + begin, 1);
            if (!result.file_name[0] || result.file_name[i] == ' ')
                break;
            result.file_type = file_operator->read_bytes(file_sectors["file type"].first + begin, file_sectors["file type"].second);
            result.cluster = 0;
            result.cluster = file_operator->read_bytes(file_sectors["high cluster"].first + begin, file_sectors["high cluster"].second);
            result.cluster <<= file_sectors["high cluster"].second * 8;
            result.cluster |= file_operator->read_bytes(file_sectors["low cluster"].first + begin, file_sectors["low cluster"].second);

            if (!is_deleted(result) && result.cluster == file.cluster){
                /* 找到这个文件 */
                file_operator->write_bytes(filenameinfo.first + begin, 1, DELETED_TYPE);

                while(true){
                    /* 删除目录项之前的长文件名 */
                    sub_1(clusters_index, cluster_item_index, dbr_info);

                    begin = (dbr_info.cluster_size * (clusters[clusters_index] - dbr_info.root_cluster)
                          + dbr_info.reserved_section_count + dbr_info.table_count * dbr_info.table_section_count) * dbr_info.section_size;
                    begin += cluster_item_index * EVERY_ITEM_LENGTH;
                    unsigned char ibyte = (unsigned char)file_operator->read_bytes(begin, sizeof(char));
                    unsigned char xbyte = (unsigned char)file_operator->read_bytes(file_sectors["file type"].first + begin, file_sectors["file type"].second);
                    if (is_deleted(ibyte)){
                        /*如果是删除项一定不是长目录项*/
                        break;
                    }
                    else if (!is_long(xbyte)){
                        /*如果属性位不是0x0f一定不是长目录项*/
                        break;
                    }
                    file_operator->write_bytes(begin, sizeof(unsigned char), DELETED_TYPE);
                }
                return;
            }

            /*修改指针*/
            add_1(clusters_index, cluster_item_index, clusters, dbr_info);
        }
        throw int();
    }

    inline unsigned file_sectors_begin(const char * str){
        return file_sectors[str].first;
    }
    inline unsigned file_sectors_length(const char * str){
        return file_sectors[str].second;
    }

    DBR get_dbr(){
        return dbr_info;
    }
};

#endif
