#include "Sector.h"
#include "DirectRead.h"
#include "FAT32.h"
#include "MultiByte.h"
#include <string>
#include <iostream>

#ifndef FAT32_file_H
#define FAT32_file_H
#define DELETED_FILE_NAME0 0xe5
#define INVALID_FILE_TYPE 0xf
#define INVALID_FILE_CLUSTER 0x0fffffff
#define EVERY_ITEM 0x20

read_sector file_sectors[] = {
	{ 0x00, 8, "file name" },
	{ 0x08, 3, "extra name" },
	{ 0x0b, 1, "file type" },
	{ 0x10, 2, "create time" },
	{ 0x12, 2, "last read time" },
	{ 0x14, 2, "high cluster" },
	{ 0x1a, 2, "low cluster" },
	{ 0x1c, 4, "file size" }
};

void print_root(unsigned begin) {
	for (unsigned i = 0; i < sizeof(file_sectors) / sizeof(read_sector); ++i){
		read_sector now_reading = file_sectors[i];
		if (now_reading.size == 2 || now_reading.size == 4) {
			unsigned b = read_bytes(now_reading.begin_byte + begin, now_reading.size);
			std::cout << now_reading.name << ":" << b << std::endl;
		}
	}
}

typedef struct {
	char filename[9];
	char extraname[4];
	unsigned char file_type;
	unsigned create_time;
	unsigned last_read_time;
	unsigned cluster;
	unsigned size;
	std::string long_filename;
}FAT32_file;

void wstrcpy(wchar_t * dest, const wchar_t * src){
	while (*dest++ = *src++);
}

FAT32_file get_file(FAT32 fat_info, unsigned begin) {
	FAT32_file result;
	read_sector now_reading = file_sectors[0];
	unsigned i = 0;
	for (i = 0; i < now_reading.size; ++i){
		result.filename[i] = read_bytes(now_reading.begin_byte + begin + i, 1);
		if (result.filename[i] == ' ') break;
	}
	result.filename[i] = 0;
	now_reading = file_sectors[1];
	for (i = 0; i < now_reading.size; ++i){
		result.extraname[i] = read_bytes(now_reading.begin_byte + begin + i, 1);
		if (result.extraname[i] == ' ') break;
	}
	result.extraname[i] = 0;

	result.file_type = read_bytes(file_sectors[2].begin_byte + begin, file_sectors[2].size);
	result.create_time = read_bytes(file_sectors[3].begin_byte + begin, file_sectors[3].size);
	result.last_read_time = read_bytes(file_sectors[4].begin_byte + begin, file_sectors[4].size);
	result.size = read_bytes(file_sectors[7].begin_byte + begin, file_sectors[7].size);
	result.cluster = 0;
	result.cluster = read_bytes(file_sectors[5].begin_byte + begin, file_sectors[5].size);
	result.cluster <<= file_sectors[6].size * 8;
	result.cluster |= read_bytes(file_sectors[6].begin_byte + begin, file_sectors[6].size);

	if (result.file_type != INVALID_FILE_TYPE){
		std::wstring temp_long_filename;
		temp_long_filename[0] = 0;
		unsigned begin_limit = (begin - fat_info.reserved_section_count * SECTION_SIZE) / (fat_info.cluster_size * SECTION_SIZE);
		begin_limit = begin_limit * (fat_info.cluster_size * SECTION_SIZE) + fat_info.reserved_section_count * SECTION_SIZE;
		while (begin >= begin_limit){
			begin -= EVERY_ITEM;

			/* check type */
			if (read_bytes(file_sectors[2].begin_byte + begin, file_sectors[2].size) != INVALID_FILE_TYPE){
				break;
			}

			unsigned long_begin[] = { 0x1, 0xe, 0x1c }, long_end[] = { 0xa, 0x19, 0x1f };
			for (unsigned i = 0; i < sizeof(long_begin) / sizeof(unsigned); ++i){
				for (unsigned j = long_begin[i]; j < long_end[i]; j += sizeof(wchar_t)){
					wchar_t temp_char = read_bytes(j + begin, sizeof(wchar_t));
					temp_long_filename += tolower(temp_char);
				}
			}
		}
		if (!temp_long_filename[0]) {
			for (unsigned i = 0; result.filename[i]; ++i)
				temp_long_filename += tolower(wchar_t(result.filename[i]));
			if (result.extraname[0]){
				temp_long_filename += L".";
				for (unsigned i = 0; result.extraname[i]; ++i)
					temp_long_filename += tolower(wchar_t(result.extraname[i]));
			}
		}
		result.long_filename = wstring2string(temp_long_filename);
	}
	return result;
}

#endif