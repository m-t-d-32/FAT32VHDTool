#ifndef DEFINES_H
#define DEFINES_H

#include "FAT32_file.h"
#include <cstring>

#define LAST_DIR ".."
#define CURRENT_DIR "."

#define INVALID_FILE_CLUSTER 0x0FFFFFF7
#define EVERY_ITEM_LENGTH 0x20
#define DEFAULT_TABLE_COUNT 2
#define DEFAULT_ROOT_CLUSTER 2

#define FREE_FILE_CLUSTER 0x00000000
#define FIRST_CLUSTER 0x0FFFFFF8
#define SECOND_CLUSTER 0xFFFFFFFF

#define LONG_FILE_TYPE 0xf
#define FOLDER_TYPE 0x10
#define DELETED_TYPE 0xe5
#define VOLUME_TYPE 0x8
#define LONG_END_TYPE 0xff
#define EVERY_GB 1024
#define EVERY_MB 1024
#define EVERY_KB 1024

inline bool is_folder(FAT32_file file){ return file.file_type == FOLDER_TYPE; }
inline bool is_deleted(FAT32_file file){ return ((unsigned char)file.file_name[0]) == DELETED_TYPE; }
inline bool is_last_dir(FAT32_file file){ return !strcmp(file.file_name, LAST_DIR); }
inline bool is_current_dir(FAT32_file file){ return !strcmp(file.file_name, CURRENT_DIR); }
inline bool is_volume(FAT32_file file) { return file.file_type == VOLUME_TYPE; }
inline bool is_long(FAT32_file file){ return file.file_type == LONG_FILE_TYPE; }
inline bool is_file(FAT32_file file){ return !is_folder(file) && !is_deleted(file)
            && !is_long(file) && !is_volume(file);}
inline bool is_long_end(unsigned short t){return !t;}
inline bool is_deleted(unsigned char t){return t == DELETED_TYPE;}
inline bool is_long(unsigned char t){return t == LONG_FILE_TYPE;}
inline bool is_invalid_cluster(unsigned t){return t >= INVALID_FILE_CLUSTER;}
inline bool is_last_long(unsigned t){return t & 0x40;}

inline void rstrip(char * str){
    int len = strlen(str), i = 0;
    for (i = 0; i < len && str[i] != ' '; ++i);
    for (; i < len; ++i) str[i] = 0;
}

#endif // DEFINES_H
