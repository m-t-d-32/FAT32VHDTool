#ifndef EXTRACT_H
#define EXTRACT_H

#include "DBROperator.h"
#include "FileOperator.h"
#include "FAT32FileReader.h"
#include "DEFINES.h"
#include "Tree.h"
#include <direct.h>
#include <QDir>
#include <QByteArray>
#include <QMessageBox>
#include <QIODevice>

class FileExtracter{
public:
    static void extract_file(QString destFile, Tree::Node * node, DBROperator * reader){
        FAT32_file fileinfo = node->file;
        if (is_file(node->file)){
            QFile * f = new QFile(destFile);
            if (!f->open(QIODevice::ReadWrite)){
                delete f;
                QMessageBox::warning(nullptr, "写入失败！", "写入文件" + destFile + "失败！");
                return;
            }
            DBR dbr_info = reader->get_dbr();

            char * buffer = new char[dbr_info.cluster_size * dbr_info.section_size];
            while (fileinfo.cluster != INVALID_FILE_CLUSTER){
                unsigned begin = (dbr_info.cluster_size * (fileinfo.cluster - dbr_info.root_cluster)
                + dbr_info.reserved_section_count + dbr_info.table_count * dbr_info.table_section_count) * dbr_info.section_size;
                unsigned size = dbr_info.cluster_size * dbr_info.section_size >
                        fileinfo.size ? fileinfo.size : dbr_info.cluster_size * dbr_info.section_size;
                reader->getDirectReader()->read_blocks(begin, size, buffer);
                f->write(buffer, size);

                fileinfo.size -= size;
                if (fileinfo.size <= 0){
                    break;
                }
                fileinfo.cluster = reader->get_next_cluster(fileinfo.cluster);
            }
            delete []buffer;
            f->close();
            delete f;
        }
        else if (is_folder(node->file)){
            QDir * d = new QDir();
            if (!d->exists(destFile)){
                bool ok = d->mkdir(destFile);
                delete d;
                if(!ok){
                    QMessageBox::warning(nullptr, "创建文件夹", "文件夹" + destFile + "创建失败！");
                    return;
                }
            }
            for (Tree::Node * child: node->children){
                QString qs = child->file.long_filename;
                QString path = QDir(destFile).filePath(qs);
                extract_file(path, child, reader);
            }
        }
    }
};

#endif // EXTRACT_H
