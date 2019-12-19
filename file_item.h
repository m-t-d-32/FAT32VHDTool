#ifndef FILEITEM_H
#define FILEITEM_H

#include <QObject>
#include <QStandardItem>
#include "tree.h"

class FileItem : public QStandardItem
{
public:
    FileItem(Tree::Node *);

    Tree::Node * get_node();
private:
    Tree::Node * treenode;
};

#endif // FILEITEM_H
