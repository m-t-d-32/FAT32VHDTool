#include <iostream>
#include "FAT32_file.h"
#include <vector>

#ifndef TREE_H
#define TREE_H

#define LAST_DIR ".."
#define CURRENT_DIR "."
extern FAT32 fat_info;
extern char TEST_VOLUME[];

typedef struct treenode {
	FAT32_file file;
	std::vector<struct treenode *> children;
} Node;

Node * create_tree(FAT32_file now_file, std::vector<FAT32_file> dir_files) {
	Node * result = new Node();
	result->file = now_file;
	for (unsigned i = 0; i < dir_files.size(); ++i){
		FAT32_file current_file = dir_files[i];
		if (strcmp(current_file.filename, CURRENT_DIR) 
			&& strcmp(current_file.filename, LAST_DIR) 	
			&& current_file.filename[0] != (char) DELETED_FILE_NAME0){
			if (current_file.file_type != INVALID_FILE_TYPE){
				if (current_file.file_type & 0x10){
					unsigned current_cluster = current_file.cluster;
					std::vector<FAT32_file> next_dir_files;
					while (current_cluster != INVALID_FILE_CLUSTER) {
						unsigned begin = (fat_info.cluster_size * (current_cluster - DEFAULT_ROOT_CLUSTER_BEGIN)
						+ fat_info.reserved_section_count + fat_info.table_count * fat_info.table_section_count) * SECTION_SIZE;
						for (unsigned j = 0; j < fat_info.cluster_size * SECTION_SIZE; j += EVERY_ITEM){
							FAT32_file temp_file = get_file(fat_info, begin + j);
							if (!temp_file.filename[0]) break;
							next_dir_files.push_back(temp_file);
						}
						current_cluster = get_next_cluster(fat_info, current_cluster);
					}					
					result->children.push_back(create_tree(current_file, next_dir_files));
				}
				else {
					Node * child = new Node();
					child->file = current_file;
					result->children.push_back(child);
				}
			}
		}
	}
	return result;
}

unsigned depth = 0;
void print_tree(Node * root){
	FAT32_file current_file = root->file;
	for (unsigned i = 0; i < depth; ++i){
		std::cout << "-";
	}
	std::cout << current_file.long_filename << std::endl;
	
	if (current_file.file_type & 0x10){
		++depth;
		std::vector<Node *> current_children = root->children;
		for (unsigned i = 0; i < current_children.size(); ++i){
			print_tree(current_children[i]);
		}
		--depth;
	}
}

std::vector<std::string> directory_trace;

void find_from_tree(Node * root, std::string filename){
	FAT32_file current_file = root->file;
	if (current_file.long_filename.find(filename) != std::string::npos){
		std::cout << TEST_VOLUME;
		for (unsigned i = 1; i < directory_trace.size(); ++i){
			std::cout << directory_trace[i] << "\\";
		}
		std::cout << current_file.long_filename << std::endl;
	}
	if (current_file.file_type & 0x10){
		std::vector<Node *> current_children = root->children;
		directory_trace.push_back(current_file.long_filename);
		for (unsigned i = 0; i < current_children.size(); ++i){
			find_from_tree(current_children[i], filename);
		}
		directory_trace.pop_back();
	}
}
#endif