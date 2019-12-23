#include <mainwindow.h>
#include <ui_mainwindow.h>
#include <QFileDialog>
#include <QDebug>
#include <file_operator.h>
#include <tree.h>
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <queue>
#include <QMessageBox>
#include <QByteArray>
#include <timer.h>
#include <file_creator.h>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QString("虚拟硬盘"));

    model = new QStandardItemModel(ui->treeView);
    ui->treeView->setModel(model);

    direct_operator = nullptr;
    dbr_operator = nullptr;
    tree = nullptr;
}

void MainWindow::resizeEvent(QResizeEvent *){
    reset_tree_item_width();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_newFile_triggered()
{
    FileCreator * creator = new FileCreator();
    creator->exec();
    if (creator->get_accepted()){
        QString filename = QFileDialog::getSaveFileName(this,
                  "保存虚拟硬盘文件", nullptr, "虚拟硬盘文件(*.c51);;所有文件(*)");
        if (filename.length() > 0){
            if (direct_operator){
                delete direct_operator;
                direct_operator = nullptr;
            }
            if (dbr_operator){
                delete dbr_operator;
                dbr_operator = nullptr;
            }
            try {                
                direct_operator = new FileOperator(filename, true);
                dbr_operator = new DBROperator();
                dbr_operator->init_dbr(creator->get_section_size(), creator->get_cluster_size(), creator->get_create_size(), direct_operator);
                QMessageBox::information(nullptr, "成功", "创建文件成功");
                fflush();
            } catch (...) {
                QMessageBox::critical(nullptr, "错误", "创建文件失败，请检查权限或硬盘空间！");
            }
        }
    }
}

void MainWindow::set_tree(Tree::Node * root, FileItem * item){
    for (unsigned i = 0; i < root->children.size(); ++i){
        auto it = root->children[i];
        FileItem * new_item = new FileItem(it);
        item->appendRow(new_item);
        if (is_file(it->file)){
            item->setChild(i, 1, new QStandardItem(to_preferred_size(it->file.size)));
            item->setChild(i, 2, new QStandardItem(Timer::getTime(it->file.create_date,
                                                                     it->file.create_time,
                                                                     it->file.create_mms)));
            item->setChild(i, 3, new QStandardItem(Timer::getTime(it->file.modified_date,
                                                                     it->file.modified_time,
                                                                     0)));
        }
        set_tree(it, new_item);
    }
}

void MainWindow::on_openFile_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
            "请选择要打开的文件", nullptr, "虚拟硬盘文件(*.c51);;所有文件(*)");

    if (filename.length() <= 0){
        return;
    }

    if (direct_operator){
        delete direct_operator;
        direct_operator = nullptr;
    }
    if (dbr_operator){
        delete dbr_operator;
        dbr_operator = nullptr;
    }
    if (tree){
        delete tree;
        tree = nullptr;
    }

    try {
        direct_operator = new FileOperator(filename, false);
        dbr_operator = new DBROperator();
        dbr_operator->init_dbr(direct_operator);
        dbr_operator->verify_fat32();
    } catch (INVALID_SRC_FILE) {
        QMessageBox::critical(nullptr, "错误", "无法打开此文件！");
        return;
    } catch (INVALID_FAT32_SYSTEM) {
        QMessageBox::critical(nullptr, "错误", "不是一个合法的FAT32存储文件！");
        return;
    }

    fflush();
}

void MainWindow::reset_tree_item_width(){
    ui->treeView->setColumnWidth(0, ui->treeView->width() / 4);
    ui->treeView->setColumnWidth(1, ui->treeView->width() / 4);
    ui->treeView->setColumnWidth(2, ui->treeView->width() / 4);
    ui->treeView->setColumnWidth(3, ui->treeView->width() / 4);
}


void MainWindow::fflush(){
    delete tree;
    tree = new Tree(dbr_operator);
    model->clear();

    model->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("文件名")<<QStringLiteral("文件大小")
                                     <<QStringLiteral("创建时间")<<QStringLiteral("最后修改时间"));

    reset_tree_item_width();
    selected_index = model->index(-1, 1);
    //tree->print_tree();

    for (auto it: tree->getRoot()->children){
        FileItem * item = new FileItem(it);
        model->appendRow(item);
        if (is_file(it->file)){
            model->setItem(model->rowCount() - 1, 1, new QStandardItem(to_preferred_size(it->file.size)));
            model->setItem(model->rowCount() - 1, 2, new QStandardItem(Timer::getTime(it->file.create_date,
                                                                                         it->file.create_time,
                                                                                         it->file.create_mms)));
            model->setItem(model->rowCount() - 1, 3, new QStandardItem(Timer::getTime(it->file.modified_date,
                                                                                         it->file.modified_time,
                                                                                         0)));
        }
        set_tree(it, item);
    }
}

void MainWindow::on_extractButton_clicked()
{
    if (selected_index.row() < 0){
        QMessageBox::information(nullptr, "无法提取", "没有选中任何文件！");
    }
    else {
        FileItem * selected_item = (FileItem *)model->itemFromIndex(selected_index);
        QString defaultName = selected_item->get_node()->file.long_filename;
        QString filename = QFileDialog::getSaveFileName(this,
            "请选择要保存的路径", defaultName, "所有文件(*)");
        if (filename.length() > 0){
            tree->extract_file(filename, selected_item->get_node(), dbr_operator);
            QMessageBox::information(nullptr, "提取", "提取完毕！");
        }
    }
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    selected_index = index.sibling(index.row(), 0);
}

void MainWindow::on_deleteButton_clicked()
{
    if (selected_index.row() < 0){
        QMessageBox::information(nullptr, "无法删除", "没有选中任何文件！");
    }
    else {
        FileItem * selected_item = (FileItem *)model->itemFromIndex(selected_index);
        Tree::Node * node = selected_item->get_node();
        tree->delete_file(node);
        tree->commit_FAT();
        fflush();
    }    
}

void MainWindow::on_help_triggered()
{
    QMessageBox::information(nullptr, "关于产品", "这是一个简单的虚拟硬盘程序");
}

void MainWindow::on_addFileButton_clicked()
{
    if (!tree){
        QMessageBox::information(nullptr, "无法添加", "没有打开虚拟磁盘文件！");
    }
    else {
        Tree::Node * node;
        if (selected_index.row() < 0){
            node = tree->getRoot();
        }
        else {
            FileItem * selected_item = (FileItem *)model->itemFromIndex(selected_index);
            node = selected_item->get_node();
            if (!is_folder(node->file)){
                QMessageBox::critical(nullptr, "错误", "只能向文件夹中添加文件！");
                return;
            }
        }

        QStringList filenames = QFileDialog::getOpenFileNames(this, "添加文件");
        tree->begin_find_free_cluster();        
        try {
            /* 开始事务操作 */
            for (QString qs: filenames){
                tree->revert_FAT(); /* 防止之前的不一致 */
                tree->add_file(node, qs);
                tree->commit_FAT();
            }            
        } catch (INSUFFICIENT_SPACE) {
            QMessageBox::critical(nullptr, "错误", "空间不足！");
            tree->revert_FAT();
        } catch (...) {
            QMessageBox::critical(nullptr, "错误", "请检查是否被其他文件占用！");
            tree->revert_FAT();
        }
        tree->end_find_free_cluster();        
        fflush();
    }
}

void MainWindow::on_addFolderButton_clicked()
{
    if (!tree){
        QMessageBox::information(nullptr, "无法添加", "没有打开虚拟磁盘文件！");
    }
    else {
        QString foldername = QInputDialog().getText(nullptr, "新建文件夹", "请输入文件夹名称:");
        if (foldername.size() <= 0){
            QMessageBox::critical(nullptr, "错误", "文件夹名不能为空！");
        }
        else {
            Tree::Node * node;
            if (selected_index.row() < 0){
                node = tree->getRoot();
            }
            else {
                FileItem * selected_item = (FileItem *)model->itemFromIndex(selected_index);
                node = selected_item->get_node();
                if (!is_folder(node->file)){
                    QMessageBox::critical(nullptr, "错误", "只能向文件夹中添加文件夹！");
                    return;
                }
            }
            tree->revert_FAT(); /* 防止之前的不一致 */
            try {
                /* 开始事务操作 */
                tree->add_folder(node, foldername);
                tree->commit_FAT();
            } catch (...) {
                QMessageBox::critical(nullptr, "错误", "空间不足！");
                tree->revert_FAT();
            }
            fflush();
        }
    }
}
