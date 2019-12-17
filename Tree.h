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
        int depth;
        std::vector<struct treenode *> children;
    } Node;

private:

    Node * root;

    DBRReader * fat32Reader;

    FAT32FileReader * fat32FileReader;

    std::vector<std::string> directory_trace;

    void rr_print_tree(Node * root, unsigned depth){
        FAT32_file current_file = root->file;
        for (unsigned i = 0; i < depth; ++i){
            std::cerr << "-";
        }
        qDebug() << current_file.long_filename;
        qDebug() << "\n";

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

    Tree(DBRReader * dbrReader){
        this->fat32Reader = dbrReader;
        DBR dbr_info = dbrReader->get_dbr();
        FAT32_file DIRECT_ROOT_FILE = { "/", "", 0x10, 0, 0, dbr_info.root_cluster, 0, "" };
        this->fat32FileReader = new FAT32FileReader(dbrReader->getDirectReader(), dbr_info);
        root = create_tree(DIRECT_ROOT_FILE, 0);
    }

    Node * create_tree(FAT32_file current_file, int depth) {
        Node * result = new Node();
        result->file = current_file;
        result->depth = depth;
        if (is_file(current_file)){
            return result;
        }
        else if (is_folder(current_file)){
            std::vector<FAT32_file> next_dir_files;
            std::vector<unsigned> cluster_occupied;
            while (!is_invalid_cluster(current_file.cluster)){
                cluster_occupied.push_back(current_file.cluster);
                current_file.cluster = fat32Reader->get_next_cluster(current_file.cluster);
            }
            next_dir_files = fat32FileReader->get_valid_files(cluster_occupied);
            for (FAT32_file file: next_dir_files){
                result->children.push_back(create_tree(file, depth + 1));
            }
        }
        return result;
    }

    void print_tree(){
        unsigned depth = 0;
        rr_print_tree(root, depth);
    }

//    void find_from_tree(std::string filename){
//        FAT32_file current_file = root->file;
//        if (current_file.long_filename.find(filename) != std::string::npos){
//            for (unsigned i = 1; i < directory_trace.size(); ++i){
//                std::cout << directory_trace[i] << "\\";
//            }
//            std::cout << current_file.long_filename << std::endl;
//        }
//        if (current_file.file_type & 0x10){
//            std::vector<Node *> current_children = root->children;
//            directory_trace.push_back(current_file.long_filename);
//            for (unsigned i = 0; i < current_children.size(); ++i){
//                find_from_tree(filename);
//            }
//            directory_trace.pop_back();
//        }
//    }

    Node * getRoot(){
        return root;
    }

};


#endif
