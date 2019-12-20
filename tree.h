#include <iostream>
#include <fat32_file_reader.h>
#include <defines.h>
#include <vector>
#include <QDebug>
#include <string>
#include <QFileInfo>
#include <cmath>
#include <algorithm>
#include <timer.h>

#ifndef TREE_H
#define TREE_H

typedef struct {

} INSUFFICIENT_SPACE;

class Tree {

public:

    typedef struct treenode {
        FAT32_file file;
        struct treenode * parent;
        std::vector<struct treenode *> children;
    } Node;

private:

    Node * root;

    DBROperator * dbr_operator;

    FAT32FileReader * fat32_file_reader;

    unsigned free_cluster_begin;

    void rr_print_tree(Node * root, unsigned depth){
        FAT32_file current_file = root->file;
        for (unsigned i = 0; i < depth; ++i){
            std::cerr << "-";
        }
        qDebug() << current_file.long_filename;

        if (current_file.file_type & 0x10){
            ++depth;
            std::vector<Node *> current_children = root->children;
            for (unsigned i = 0; i < current_children.size(); ++i){
                rr_print_tree(current_children[i], depth);
            }
            --depth;
        }
    }

public:

    Tree(DBROperator * dbrReader){
        this->dbr_operator = dbrReader;
        DBR dbr_info = dbrReader->get_dbr();
        FAT32_file DIRECT_ROOT_FILE(FOLDER_TYPE, dbr_info.root_cluster, 1, "");
        this->fat32_file_reader = new FAT32FileReader(dbrReader->get_file_operator(), dbr_info);
        root = create_tree(DIRECT_ROOT_FILE);
        root->parent = nullptr;
    }

    Node * create_tree(FAT32_file current_file) {
        Node * result = new Node();
        result->file = current_file;
        if (is_file(current_file)){
            return result;
        }
        else if (is_folder(current_file)){
            std::vector<FAT32_file> next_dir_files;
            std::vector<unsigned> cluster_occupied;
            while (!is_invalid_cluster(current_file.cluster)){
                cluster_occupied.push_back(current_file.cluster);
                current_file.cluster = dbr_operator->get_next_cluster(current_file.cluster);
            }
            next_dir_files = fat32_file_reader->get_valid_files(cluster_occupied);
            for (FAT32_file file: next_dir_files){
                Node * child = create_tree(file);
                child->parent = result;
                result->children.push_back(child);
            }
        }
        return result;
    }

    void begin_find_free_cluster(){
        /* 性能 */
        DBR dbr = dbr_operator->get_dbr();
        unsigned fat_begin = dbr.reserved_section_count * dbr.section_size;
        unsigned cluster_count = dbr.cluster_count;
        for (unsigned i = 0; i < cluster_count; ++i){
            unsigned read_fat = dbr_operator->get_file_operator()->read_bytes(fat_begin + i * FAT_ITEM_SIZE, FAT_ITEM_SIZE);
            if (!read_fat){
                free_cluster_begin = i;
                return;
            }
        }
        free_cluster_begin = 0;
    }

    void revert_FAT(){
        /* 如果添加过程出现问题，从FAT2恢复到FAT1 */
        DBR dbr = dbr_operator->get_dbr();
        unsigned size = dbr.table_section_count * dbr.section_size;
        unsigned fat1_begin = dbr.reserved_section_count * dbr.section_size;
        unsigned fat2_begin = fat1_begin + size;

        char * buffer = new char[size];
        dbr_operator->get_file_operator()->read_blocks(fat2_begin, size, buffer);
        dbr_operator->get_file_operator()->write_blocks(fat1_begin, size, buffer);
        delete []buffer;
    }
    void commit_FAT(){
        /* 如果没有问题，从FAT1替换到FAT2 */
        DBR dbr = dbr_operator->get_dbr();
        unsigned size = dbr.table_section_count * dbr.section_size;
        unsigned fat1_begin = dbr.reserved_section_count * dbr.section_size;
        unsigned fat2_begin = fat1_begin + size;

        char * buffer = new char[size];
        dbr_operator->get_file_operator()->read_blocks(fat1_begin, size, buffer);
        dbr_operator->get_file_operator()->write_blocks(fat2_begin, size, buffer);
        delete []buffer;
    }

    void add_folder(Node * parent, QString foldername){
        /* 只是添加一个空文件夹，父文件夹是parent */

        DBR dbr = fat32_file_reader->get_dbr();
        unsigned fat_begin = dbr.reserved_section_count * dbr.section_size;
        begin_find_free_cluster();
        if (!free_cluster_begin){
            throw INSUFFICIENT_SPACE{};
        }
        else {
            dbr_operator->get_file_operator()->write_bytes(fat_begin + free_cluster_begin * FAT_ITEM_SIZE, FAT_ITEM_SIZE, INVALID_FILE_CLUSTER);
        }

        unsigned cluster_begin = fat_begin + dbr.table_count * dbr.table_section_count * dbr.section_size;
        cluster_begin -= dbr.root_cluster * dbr.cluster_size * dbr.section_size;
        unsigned size_every = dbr.cluster_size * dbr.section_size;
        unsigned cluster = free_cluster_begin;
        unsigned dest_begin = cluster_begin + cluster * size_every;

        /* 将.和..写入簇 */
        /* do nothing */
        /* 这里不提供实现，因为FAT32中不一定存在.和.. */

        /* 清空簇中的内容 */
        char * buffer = new char[size_every];
        memset(buffer, 0, size_every);
        dbr_operator->get_file_operator()->write_blocks(dest_begin, size_every, buffer);
        delete []buffer;

        add_to_parent(parent, foldername, cluster, 0, FOLDER_TYPE);
        end_find_free_cluster();
    }

    void add_file(Node * parent, QString filepath){
        QString filename = get_filename_without_path(filepath);
        /* 只是添加普通文件，父目录是node */
        if (!free_cluster_begin){
            throw INSUFFICIENT_SPACE{};
        }

        /* 获取需要占几个簇，并写入FAT表 */
        std::vector<unsigned> occupy_clusters;
        FileOperator src_operator(filepath);
        unsigned size = src_operator.get_file_size();
        DBR dbr = dbr_operator->get_dbr();
        unsigned fat_begin = dbr.reserved_section_count * dbr.section_size;
        unsigned cluster_begin = fat_begin + dbr.table_count * dbr.table_section_count * dbr.section_size;
        cluster_begin -= dbr.root_cluster * dbr.cluster_size * dbr.section_size;
        unsigned size_every = dbr.cluster_size * dbr.section_size;

        unsigned cluster_count = size / size_every + !!(size % size_every);
        if (!cluster_count)
            cluster_count = 1; /* 修复空文件bug */

        unsigned last_free_cluster = 0;
        for (unsigned i = 0; i < cluster_count; ++i){
            bool find = false;
            while (free_cluster_begin < dbr.cluster_count){
                unsigned read_fat = dbr_operator->get_file_operator()->read_bytes(fat_begin + free_cluster_begin * FAT_ITEM_SIZE, FAT_ITEM_SIZE);
                if (!read_fat){
                    find = true;
                    break;
                }
                ++free_cluster_begin;
            }
            if (!find){
                throw INSUFFICIENT_SPACE{};
            }
            else {
                if (last_free_cluster){
                    dbr_operator->get_file_operator()->write_bytes(fat_begin + last_free_cluster * FAT_ITEM_SIZE, FAT_ITEM_SIZE, free_cluster_begin);
                }
                last_free_cluster = free_cluster_begin;
                occupy_clusters.push_back(free_cluster_begin++);
            }
        }
        dbr_operator->get_file_operator()->write_bytes(fat_begin + last_free_cluster * FAT_ITEM_SIZE, FAT_ITEM_SIZE, INVALID_FILE_CLUSTER);

        /* 将文件写入簇 */
        char * buffer = new char[size_every];
        for (unsigned i = 0; i < occupy_clusters.size(); ++i){
            unsigned cluster = occupy_clusters[i];
            unsigned dest_begin = cluster_begin + cluster * size_every;
            unsigned src_begin = i * size_every;

            if (i != occupy_clusters.size() - 1){
                src_operator.read_blocks(src_begin, size_every, buffer);
                dbr_operator->get_file_operator()->write_blocks(dest_begin, size_every, buffer);
            }
            else {
                unsigned remains = size - cluster_count * size_every;
                memset(buffer, 0, size_every);
                src_operator.read_blocks(src_begin, remains, buffer);
                dbr_operator->get_file_operator()->write_blocks(dest_begin, remains, buffer);
            }
        }
        delete []buffer;

        add_to_parent(parent, filename, occupy_clusters[0], size, ORDINARY_FILE_TYPE);
    }

    void add_to_parent(Node * parent, QString filename,
                       unsigned occupy_cluster_begin, unsigned size,
                       unsigned file_type){
        if (!parent){
            return;
        }
        /* 生成文件各个目录项 */
        /* assert 每个目录项 == 32Byte */
        DBR dbr = dbr_operator->get_dbr();
        unsigned fat_begin = dbr.reserved_section_count * dbr.section_size;
        unsigned cluster_begin = fat_begin + dbr.table_count * dbr.table_section_count * dbr.section_size;
        cluster_begin -= dbr.root_cluster * dbr.cluster_size * dbr.section_size;
        unsigned size_every = dbr.cluster_size * dbr.section_size;

        unsigned long_begin[] = { 0x1, 0xe, 0x1c }, long_end[] = { 0xb, 0x1a, 0x20 };
        unsigned item_name_size = 0;
        for (unsigned i = 0; i < sizeof(long_begin) / sizeof(long_begin[0]); ++i){
            item_name_size += long_end[i] - long_begin[i];
        }
        std::u16string fileUTF16name = filename.toStdU16String();
        fileUTF16name.append(1, (unsigned short)0);
        unsigned long_count = fileUTF16name.size() * sizeof(unsigned short) / item_name_size + (!!(fileUTF16name.size() * sizeof(unsigned short) % item_name_size));    /*ceil*/
        std::vector<std::vector<unsigned char> > allitems;
        /* 长目录项 */
        for (unsigned i = 0; i < long_count; ++i){
            std::vector<unsigned char> item;
            item.reserve(EVERY_ITEM_LENGTH);

            if (i == long_count - 1){
                item.push_back(LAST_LONG);
            }
            else {
                item.push_back((unsigned char)(i + 1)); /*从1开始*/
            }
            for (unsigned j = 0; j < long_end[0] - long_begin[0]; j += sizeof(unsigned short)){
                /* a >> 1 和 a / 2相同，类似我们可以知道 a & 1 与 a % 2相同 */
                if ((i * item_name_size + j) >> 1 >= fileUTF16name.size()){
                    item.push_back(LONG_END_TYPE);
                    item.push_back(LONG_END_TYPE);
                }
                else {
                    item.push_back((unsigned char)(fileUTF16name[(i * item_name_size + j) >> 1] & 0xff));
                    item.push_back((unsigned char)(fileUTF16name[(i * item_name_size + j) >> 1] >> 8));
                }
            }
            item.push_back(LONG_FILE_TYPE);
            item.push_back(0);
            item.push_back(0);
            for (unsigned j = 0; j < long_end[1] - long_begin[1]; j += 2){
                if ((i * item_name_size + long_end[0] - long_begin[0] + j) >> 1 >= fileUTF16name.size()){
                    item.push_back(LONG_END_TYPE);
                    item.push_back(LONG_END_TYPE);
                }
                else {
                    item.push_back((unsigned char)(fileUTF16name[(i * item_name_size + long_end[0] - long_begin[0] + j) >> 1] & 0xff));
                    item.push_back((unsigned char)(fileUTF16name[(i * item_name_size + long_end[0] - long_begin[0] + j) >> 1] >> 8));
                }
            }
            item.push_back(0);
            item.push_back(0);
            for (unsigned j = 0; j < long_end[2] - long_begin[2]; j += 2){
                if ((i * item_name_size + long_end[0] - long_begin[0] + long_end[1] - long_begin[1] + j) >> 1 >= fileUTF16name.size()){
                    item.push_back(LONG_END_TYPE);
                    item.push_back(LONG_END_TYPE);
                }
                else {
                    item.push_back((unsigned char)(fileUTF16name[(i * item_name_size + long_end[0] - long_begin[0] + long_end[1] - long_begin[1] + j) >> 1] & 0xff));
                    item.push_back((unsigned char)(fileUTF16name[(i * item_name_size + long_end[0] - long_begin[0] + long_end[1] - long_begin[1] + j) >> 1] >> 8));
                }
            }
            allitems.insert(allitems.begin(), item);
        }
        /* 短目录项 */
        {
            std::vector<unsigned char> item;
            unsigned create_date, create_time, create_mm10s;
            unsigned modified_date, modified_time, modified_mm10s;
            Timer::setNowTime(create_date, create_time, create_mm10s);
            Timer::setNowTime(modified_date, modified_time, modified_mm10s);

            item.reserve(EVERY_ITEM_LENGTH);
            std::string short_filename = get_short_filename(filename, fat32_file_reader->get_file_sectors()["file name"].second);
            for (unsigned i = 0; i < short_filename.size(); ++i){
                item.push_back((unsigned char)(short_filename.at(i)));
            }
            QStringList dottedstrs = filename.split(".");
            std::string short_extraname = dottedstrs.size() > 1 ? get_short_filename(
                  dottedstrs[dottedstrs.size() - 1],  fat32_file_reader->get_file_sectors()["extra name"].second) : "___";
            for (unsigned i = 0; i < short_extraname.size(); ++i){
                item.push_back((unsigned char)(short_extraname.at(i)));
            }
            item.push_back(file_type & 0xff);
            item.push_back(0);  /*0c*/
            item.push_back((unsigned char)(create_mm10s & 0xff));   /*0d*/            
            item.push_back((unsigned char)(create_time & 0xff));   /*0e*/
            item.push_back((unsigned char)(create_time >> 8));   /*0f*/
            item.push_back((unsigned char)(create_date & 0xff));   /*10*/
            item.push_back((unsigned char)(create_date >> 8));   /*11*/
            item.push_back((unsigned char)(create_date & 0xff));   /*12*/
            item.push_back((unsigned char)(create_date >> 8));   /*13*/

            item.push_back((unsigned char)(occupy_cluster_begin >> 16 & 0xff));
            item.push_back((unsigned char)(occupy_cluster_begin >> 24 & 0xff));

            item.push_back((unsigned char)(modified_time & 0xff));   /*16*/
            item.push_back((unsigned char)(modified_time >> 8));   /*17*/
            item.push_back((unsigned char)(modified_date & 0xff));   /*18*/
            item.push_back((unsigned char)(modified_date >> 8));   /*19*/

            item.push_back((unsigned char)(occupy_cluster_begin & 0xff));
            item.push_back((unsigned char)(occupy_cluster_begin >> 8 & 0xff));

            item.push_back((unsigned char)(size & 0xff));
            item.push_back((unsigned char)(size >> 8 & 0xff));
            item.push_back((unsigned char)(size >> 16 & 0xff));
            item.push_back((unsigned char)(size >> 24 & 0xff));
            allitems.push_back(item);
        }

        /* 插入父目录目录项 */
        /* 目录本身碎片化了 */
        std::vector<unsigned> parent_occupy_clusters;
        unsigned current_cluster = parent->file.cluster;
        while (!is_invalid_cluster(current_cluster)){
            parent_occupy_clusters.push_back(current_cluster);
            current_cluster = dbr_operator->get_next_cluster(current_cluster);
        }

        unsigned clusters_index = 0;
        unsigned cluster_item_index = 0;
        unsigned can_insert_count = 0;
        while (true){
            unsigned begin = cluster_begin + parent_occupy_clusters[clusters_index] * size_every;
            begin += cluster_item_index * EVERY_ITEM_LENGTH;
            unsigned char test_byte = (unsigned char)dbr_operator->get_file_operator()->read_bytes(begin, 1);
            if (!is_can_insert(test_byte)){
                can_insert_count = 0;
            }
            else {
                can_insert_count++;
                if (can_insert_count >= allitems.size()){
                    break;
                }
            }

            if (!add_1(clusters_index, cluster_item_index, parent_occupy_clusters, dbr)){
                break;
            }
        }
        if (can_insert_count < allitems.size()){
            /* 试图增加一个簇，最多一个簇 */
            bool find = false;
            while (free_cluster_begin < dbr.cluster_count){
                unsigned read_fat = dbr_operator->get_file_operator()->read_bytes(fat_begin + free_cluster_begin * FAT_ITEM_SIZE, FAT_ITEM_SIZE);
                if (!read_fat){
                    /* 加到尾部 */
                    find = true;
                    unsigned last_free_cluster = parent_occupy_clusters.back();
                    dbr_operator->get_file_operator()->write_bytes(fat_begin + last_free_cluster * FAT_ITEM_SIZE, FAT_ITEM_SIZE, free_cluster_begin);
                    dbr_operator->get_file_operator()->write_bytes(fat_begin + free_cluster_begin * FAT_ITEM_SIZE, FAT_ITEM_SIZE, INVALID_FILE_CLUSTER);
                    parent_occupy_clusters.push_back(free_cluster_begin);
                    /* 这里需要把申请到的簇填充0 */
                    unsigned dest_begin = cluster_begin + free_cluster_begin * size_every;
                    char * buffer = new char[size_every];
                    memset(buffer, 0, size_every);
                    dbr_operator->get_file_operator()->write_blocks(dest_begin, size_every, buffer);
                    delete []buffer;

                    break;
                }
                ++free_cluster_begin;
            }
            if (!find){
                throw INSUFFICIENT_SPACE{};
            }
            /* 调回去 */
            sub_1(clusters_index, cluster_item_index, dbr);
        }
        /* 调整到开始插入的地址 */
        if (can_insert_count <= 0){
            add_1(clusters_index, cluster_item_index, parent_occupy_clusters, dbr);
        }
        else {
            for (unsigned i = 0; i != can_insert_count - 1; ++i){
                /* 减一的操作只需要做n-1次 */
                sub_1(clusters_index, cluster_item_index, dbr);
            }
        }
        /* 写入 */
        for (unsigned i = 0; i < allitems.size(); ++i){
            unsigned begin = cluster_begin + parent_occupy_clusters[clusters_index] * size_every;
            begin += cluster_item_index * EVERY_ITEM_LENGTH;
            dbr_operator->get_file_operator()->write_blocks(begin, EVERY_ITEM_LENGTH, allitems[i].data());
            add_1(clusters_index, cluster_item_index, parent_occupy_clusters, dbr);
        }
    }

    void end_find_free_cluster(){
        free_cluster_begin = 0;
    }

    void print_tree(){
        unsigned depth = 0;
        rr_print_tree(root, depth);
    }

    FAT32FileReader * get_fat32_file_reader(){
        return fat32_file_reader;
    }

    Node * getRoot(){
        return root;
    }

    void delete_from_FAT_table(Node * node){
        unsigned current_cluster = node->file.cluster;
        /*删除FAT表字段*/
        while (!is_invalid_cluster(current_cluster)){
            unsigned next_cluster = dbr_operator->get_next_cluster(current_cluster);
            dbr_operator->delete_cluster(current_cluster);
            current_cluster = next_cluster;
        }
    }

    void delete_from_parent(Node * node){
        /*删除父目录字段*/
        std::vector<FAT32_file> next_dir_files;
        std::vector<unsigned> cluster_occupied;
        FAT32_file parent_file = node->parent->file;
        while (!is_invalid_cluster(parent_file.cluster)){
            cluster_occupied.push_back(parent_file.cluster);
            parent_file.cluster = dbr_operator->get_next_cluster(parent_file.cluster);
        }
        fat32_file_reader->delete_file(cluster_occupied, node->file);
    }

    void delete_all(const std::vector<unsigned> & cluster_occupied){
        /* 删除所有文件 */
        DBR dbr_info = dbr_operator->get_dbr();
        for (auto && cluster_item: cluster_occupied){
            for (unsigned cluster_item_index = 0;
                 cluster_item_index < dbr_info.cluster_size * dbr_info.section_size / EVERY_ITEM_LENGTH;
                 ++cluster_item_index){
                unsigned begin = (dbr_info.cluster_size * (cluster_item - dbr_info.root_cluster)
                      + dbr_info.reserved_section_count + dbr_info.table_count * dbr_info.table_section_count) * dbr_info.section_size;
                begin += cluster_item_index * EVERY_ITEM_LENGTH;
                /* file_name[0]置e5，类型置零 */
                dbr_operator->get_file_operator()->write_bytes(begin + fat32_file_reader->get_file_sectors()["file name"].first, 1, DELETED_TYPE);
                dbr_operator->get_file_operator()->write_bytes(begin + fat32_file_reader->get_file_sectors()["file type"].first,
                        fat32_file_reader->get_file_sectors()["file type"].second, 0);
            }
        }
    }

    void delete_file(Node * node){
        if (is_folder(node->file)){
            std::vector<unsigned> cluster_occupied;
            unsigned now_cluster = node->file.cluster;
            while (!is_invalid_cluster(now_cluster)){
                cluster_occupied.push_back(now_cluster);
                now_cluster = dbr_operator->get_next_cluster(now_cluster);
            }
            delete_all(cluster_occupied);
        }
        delete_from_FAT_table(node);
        if (node->parent){
            delete_from_parent(node);
        }
    }

    void rr_delete(Node * root){
        if (!root){
            for (unsigned i = 0; i < root->children.size(); ++i){
                rr_delete(root->children[i]);
            }
        }
        delete root;
    }

    ~Tree(){
        rr_delete(root);
    }

};


#endif
