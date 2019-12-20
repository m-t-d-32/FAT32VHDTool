#ifndef DEFINES_H
#define DEFINES_H

#include <fat32_file.h>
#include <dbr.h>
#include <cstring>
#include <QString>
#include <string>
#include <algorithm>
#include <vector>
#include <QFileInfo>

#define LAST_DIR ".."
#define CURRENT_DIR "."

#define INVALID_FILE_CLUSTER 0x0FFFFFF7
#define EVERY_ITEM_LENGTH 0x20
#define DEFAULT_TABLE_COUNT 2
#define DEFAULT_ROOT_CLUSTER 2

#define FREE_FILE_CLUSTER 0x00000000
#define FIRST_CLUSTER 0x0FFFFFF8
#define SECOND_CLUSTER 0xFFFFFFFF
#define LAST_LONG 0x40

#define LONG_FILE_TYPE 0xf
#define ORDINARY_FILE 0x0
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
inline bool is_last_long(unsigned t){return t & LAST_LONG;}
inline bool is_can_insert(unsigned char t){return is_deleted(t) || !t;}
inline QString to_preferred_size(long size){
    float b = size;
    QString result = "B";
    if (b >= EVERY_KB){
        result = "KB";
        b /= EVERY_KB;
        if (b >= EVERY_MB){
            result = "MB";
            b /= EVERY_MB;
            if (b >= EVERY_GB){
                result = "MB";
                b /= EVERY_GB;
            }
        }
    }
    return QString::number(b) + result;
}
inline QString get_filename_without_path(QString filepath){
    return QFileInfo(filepath).fileName();
}
inline std::string get_short_filename(QString filename, unsigned maxlen){
    std::string result;
    std::u16string fileUTF16name = filename.toStdU16String();
    for (unsigned i = 0; i < std::min((unsigned)fileUTF16name.size(), maxlen); ++i){
        unsigned short it = fileUTF16name[i];
        if (it >> 8){
            result += '_';
        }
        else {
            result += (char)(it & 0xff);
        }
    }
    if (fileUTF16name.size() < maxlen){
        for (unsigned it = fileUTF16name.size(); it < maxlen; ++it)
            result += '_';
    }
    return result;
}
inline bool add_1(unsigned & clusters_index, unsigned & cluster_item_index,
                  const std::vector<unsigned> & clusters, DBR dbr_info){
    ++cluster_item_index;
    if (cluster_item_index * EVERY_ITEM_LENGTH >= dbr_info.cluster_size * dbr_info.section_size){
        cluster_item_index = 0;
        clusters_index++;
        if (clusters_index >= clusters.size()){
            return false;
        }
    }
    return true;
}
inline bool sub_1(unsigned & clusters_index, unsigned & cluster_item_index,
                  DBR dbr_info){
    if (cluster_item_index == 0){
        if (clusters_index == 0){
            return false;
        }
        else {
            --clusters_index;
            cluster_item_index = dbr_info.cluster_size * dbr_info.section_size / EVERY_ITEM_LENGTH - 1;
        }
    }
    else {
        --cluster_item_index;
    }
    return true;
}
inline void rstrip(char * str){
    int len = strlen(str), i = 0;
    for (i = 0; i < len && str[i] != ' '; ++i);
    for (; i < len; ++i) str[i] = 0;
}

#endif // DEFINES_H
