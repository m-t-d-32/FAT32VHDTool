#include "fileitem.h"

FileItem::FileItem(Tree::Node * node):QStandardItem(node->file.long_filename)
{
    this->treenode = node;
}

Tree::Node * FileItem::getNode(){
    return treenode;
}
