#include <file_item.h>

FileItem::FileItem(Tree::Node * node):QStandardItem(node->file.long_filename)
{
    this->treenode = node;
}

Tree::Node * FileItem::get_node(){
    return treenode;
}

