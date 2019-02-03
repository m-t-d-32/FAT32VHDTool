#define UNICODE
#include "FAT32.h"
#include "FAT32_file.h"
#include "Tree.h"
#include "MultiByte.h"
#include <iostream>
#include <vector>
#include <string>
#define BUFFER_SIZE 0xff

FAT32 fat_info;
char TEST_VOLUME[BUFFER_SIZE];
char TEST_FILENAME[BUFFER_SIZE];

int main(){
	/* Standard user interface */
	try{
		std::cout << "欢迎使用FAT32搜索系统，请输入分区名（如H）：" << std::endl;
		std::cin.getline(TEST_VOLUME, BUFFER_SIZE);
		strcat(TEST_VOLUME, ":");
		std::cout << "正在初始化分区索引，请稍候……" << std::endl;
		init_file(TEST_VOLUME);
		//print_sectors();
		verify_fat32();
		fat_info = get_fat();
		unsigned begin = (fat_info.cluster_size * (fat_info.root_cluster - DEFAULT_ROOT_CLUSTER_BEGIN)
			+ fat_info.reserved_section_count + fat_info.table_count * fat_info.table_section_count) * SECTION_SIZE;
		std::vector<FAT32_file> root_files;
		FAT32_file DEFAULT_ROOT_FILE = { "/", "", 0x10, 0, 0, fat_info.root_cluster, 0, "" };
		root_files.push_back(DEFAULT_ROOT_FILE);
		Node * root = create_tree(DEFAULT_ROOT_FILE, root_files);
		//print_tree(root);
		close_file();
		std::cout << "索引初始化完毕！" << std::endl;
		while (true){
			std::cout << "请输入您需要查找的文件名，然后按下回车键开始搜索，直接按下回车键退出程序！" << std::endl;
			std::cin.getline(TEST_FILENAME, BUFFER_SIZE);
			std::wstring temp_wfilename = string2wstring(TEST_FILENAME);
			for (int i = 0; i < temp_wfilename.size(); ++i){
				temp_wfilename[i] = tolower(temp_wfilename[i]);
			}
			std::string temp_filename = wstring2string(temp_wfilename);
			strcpy(TEST_FILENAME, temp_filename.c_str());
			if (!TEST_FILENAME[0])
				break;
			std::cout << "以下为搜索结果：" << std::endl;
			find_from_tree(root, TEST_FILENAME);
		}
		std::cout << "感谢您的使用，再见！" << std::endl;
		return 0;
	}
	catch (INVALID_DRIVE iex){
		printf(iex.ptr);
		return -1;
	}
	catch (INVALID_READ irx){
		printf(irx.ptr);
		return -2;
	}
}