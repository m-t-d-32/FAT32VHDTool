#include <iostream>
#include "FAT32FileReader.h"
#include "DEFINES.h"
#include <vector>
#include <QDebug>
#include <string>

#ifndef TREE_H
#define TREE_H

#ifndef LONG_FILE_TYPE
#define LONG_FILE_TYPE 0xf /*长文件名目录项*/
#endif

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
        FAT32_file DIRECT_ROOT_FILE = { "/", "", 0x10, 0, 0, dbr_info.root_cluster, 0, "" };
        this->fat32_file_reader = new FAT32FileReader(dbrReader->getDirectReader(), dbr_info);
        root = create_tree(DIRECT_ROOT_FILE);
        root->parent = NULL;
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
            for (int cluster_item_index = 0;
                 cluster_item_index < dbr_info.cluster_size * dbr_info.section_size / EVERY_ITEM_LENGTH;
                 ++cluster_item_index){
                unsigned begin = (dbr_info.cluster_size * (cluster_item - dbr_info.root_cluster)
                      + dbr_info.reserved_section_count + dbr_info.table_count * dbr_info.table_section_count) * dbr_info.section_size;
                begin += cluster_item_index * EVERY_ITEM_LENGTH;
                /* file_name[0]置e5，类型置零 */
                dbr_operator->getDirectReader()->write_bytes(begin + fat32_file_reader->get_file_sectors()["file name"].first, 1, DELETED_TYPE);
                dbr_operator->getDirectReader()->write_bytes(begin + fat32_file_reader->get_file_sectors()["file type"].first,
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

};


#endif
