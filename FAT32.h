#include <iostream>
#include "Sector.h"
#include "DirectRead.h"

#ifndef FAT32_H
#define FAT32_H
#define DEFAULT_ROOT_CLUSTER_BEGIN 2
#define FAT_ITEM_SIZE 4

read_sector dbr_sectors[] = {
	{0, 3, "jmp code"},
	{3, 8, "file system flags"},
	{0xd, 1, "cluster size"},
	{0xe, 2, "reserved section"},
	{0x10, 1, "fat table count"},
	{0x20, 4, "all section count"},
	{0x24, 4, "fat table section count"},
	{0x2c, 4, "root directory cluster"},
	{0x52, 8, "fat32 verification string"},
	{0x1fe, 2, "fat32 verification number"}
};

void print_sectors() {
	for (unsigned i = 0; i < sizeof(dbr_sectors)/sizeof(read_sector); ++i){
		read_sector now_reading = dbr_sectors[i];
		if (now_reading.size <= sizeof(unsigned)) {
			unsigned b = read_bytes(now_reading.begin_byte, now_reading.size);
			std::cout << now_reading.name << ":" << b << std::endl;
		}
	}
}

typedef struct {
	unsigned table_count;
	unsigned table_section_count;
	unsigned reserved_section_count;
	unsigned cluster_size; 
	unsigned root_cluster; /* 2 as default */
} FAT32;

void verify_fat32() {
	char verify_str[] = "FAT32   ";
	char verify_num[] = {0x55, 0xaa};
	for (unsigned i = 0; i < sizeof(verify_str) - 1; ++i) {
		char temp_num = read_bytes(dbr_sectors[8].begin_byte + i, 1);
		if (verify_str[i] != temp_num)
			throw rdr;
	}
	for (unsigned i = 0; i < sizeof(verify_num); ++i) {
		char temp_num = read_bytes(dbr_sectors[9].begin_byte + i, 1);
		if (verify_num[i] != temp_num)
			throw rdr;
	}
}

FAT32 get_fat() {
	FAT32 result;
	result.table_count = read_bytes(dbr_sectors[4].begin_byte, dbr_sectors[4].size);
	result.table_section_count = read_bytes(dbr_sectors[6].begin_byte, dbr_sectors[6].size);
	result.reserved_section_count = read_bytes(dbr_sectors[3].begin_byte, dbr_sectors[3].size);
	result.cluster_size = read_bytes(dbr_sectors[2].begin_byte, dbr_sectors[2].size);
	result.root_cluster = read_bytes(dbr_sectors[7].begin_byte, dbr_sectors[7].size);
	return result;
}

unsigned get_next_cluster(FAT32 fat_table, unsigned cluster) {
	unsigned begin = fat_table.reserved_section_count * SECTION_SIZE;
	unsigned result = read_bytes(begin + cluster * FAT_ITEM_SIZE, FAT_ITEM_SIZE);
	return result;
}

#endif