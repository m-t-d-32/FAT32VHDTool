#ifndef EXTRACT_H
#define EXTRACT_H

#include "DBRReader.h"
#include "DirectRead.h"
#include "FAT32FileReader.h"
#include "DEFINES.h"
#include "Tree.h"
#include <direct.h>
#include <QDir>
#include <QByteArray>

void extractFile(const char * destFile, Tree::Node * node, DBRReader * reader){
    FAT32_file fileinfo = node->file;
    if (is_file(node->file)){
        FILE * f = fopen(destFile, "wb");
        DBR dbr_info = reader->get_dbr();

        char * buffer = new char[dbr_info.cluster_size * dbr_info.section_size];
        while (fileinfo.cluster != INVALID_FILE_CLUSTER){
            unsigned begin = (dbr_info.cluster_size * (fileinfo.cluster - dbr_info.root_cluster)
            + dbr_info.reserved_section_count + dbr_info.table_count * dbr_info.table_section_count) * dbr_info.section_size;
            unsigned size = dbr_info.cluster_size * dbr_info.section_size >
                    fileinfo.size ? fileinfo.size : dbr_info.cluster_size * dbr_info.section_size;
            reader->getDirectReader()->read_blocks(begin, size, buffer);
            fwrite(buffer, size, 1, f);

            fileinfo.size -= size;
            if (fileinfo.size <= 0){
                break;
            }
            fileinfo.cluster = reader->get_next_cluster(fileinfo.cluster);
        }
        delete []buffer;
        fclose(f);
    }
    else if (is_folder(node->file)){
        mkdir(destFile);
        for (Tree::Node * child: node->children){
            QString qs = child->file.long_filename;
            QString path = QDir(destFile).filePath(qs);
            QByteArray array = path.toLatin1();
            extractFile(array.data(), child, reader);
        }
    }
}
#endif // EXTRACT_H
