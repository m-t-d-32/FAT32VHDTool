#ifndef FILEITEM_H
#define FILEITEM_H

#include <QObject>
#include <QStandardItem>
#include "Tree.h"

class FileItem : public QStandardItem
{
public:
    FileItem(Tree::Node *);

    Tree::Node * getNode();
private:
    Tree::Node * treenode;
};

#endif // FILEITEM_H
