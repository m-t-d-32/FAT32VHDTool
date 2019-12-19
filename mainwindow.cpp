#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include "FileOperator.h"
#include "Tree.h"
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <queue>
#include <QMessageBox>
#include <QByteArray>
#include "FileExtracter.h"
#include "createfile.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("虚拟硬盘");

    model = new QStandardItemModel(ui->treeView);
    ui->treeView->setModel(model);

    direct_operator = NULL;
    dbr_operator = NULL;
    tree = NULL;
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
                  "保存虚拟硬盘文件", nullptr, "虚拟硬盘文件(*.c51);;所有文件(*.*)");
        if (filename.length() > 0){
            direct_operator = new FileOperator(filename);
            dbr_operator = new DBROperator();
            dbr_operator->init_dbr(creator->get_section_size(), creator->get_cluster_size(), creator->get_create_size(), direct_operator);
            QMessageBox::information(NULL, "成功", "创建文件成功");
            fflush();
        }
    }//qDebug() << filename << "\n";
}

void MainWindow::set_tree(Tree::Node * root, FileItem * item){
    for (auto it: root->children){
        FileItem * new_item = new FileItem(it);
        item->appendRow(new_item);
        set_tree(it, new_item);
    }
}

void MainWindow::on_openFile_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
            "请选择要打开的文件", NULL, "虚拟硬盘文件(*.c51);;所有文件(*.*)");

    if (filename.length() <= 0){
        return;
    }

    if (direct_operator){
        delete direct_operator;
        direct_operator = NULL;
    }
    if (dbr_operator){
        delete dbr_operator;
        dbr_operator = NULL;
    }
    if (tree){
        delete tree;
        tree = NULL;
    }

    direct_operator = new FileOperator(filename.toLatin1().data());
    dbr_operator = new DBROperator();
    dbr_operator->init_dbr(direct_operator);
    try {
        dbr_operator->verify_fat32();
    } catch (INVALID_SRC_FILE) {
        QMessageBox::critical(NULL, "错误", "无法打开此文件！");
        return;
    } catch (INVALID_FAT32_SYSTEM) {
        QMessageBox::critical(NULL, "错误", "不是一个合法的FAT32存储文件！");
        return;
    }

    fflush();
}

void MainWindow::fflush(){
    delete tree;
    tree = new Tree(dbr_operator);
    model->clear();

    model->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("文件名")<<QStringLiteral("完整名"));
    selected_index = model->index(-1, 1);
    //tree->print_tree();

    for (auto it: tree->getRoot()->children){
        FileItem * item = new FileItem(it);
        model->appendRow(item);
        set_tree(it, item);
    }
}

void MainWindow::on_extractButton_clicked()
{
    if (selected_index.row() < 0){
        QMessageBox::information(NULL, "无法提取", "没有选中任何文件！");
    }
    else {
        FileItem * selected_item = (FileItem *)model->itemFromIndex(selected_index);
        QString defaultName = selected_item->get_node()->file.long_filename;
        QString filename = QFileDialog::getSaveFileName(this,
            "请选择要保存的路径", defaultName, "所有文件(*)");
        FileExtracter::extract_file(filename, selected_item->get_node(), dbr_operator);
        QMessageBox::information(NULL, "提取", "提取完毕！");
    }
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    selected_index = index;
}

void MainWindow::on_deleteButton_clicked()
{
    if (selected_index.row() < 0){
        QMessageBox::information(NULL, "无法删除", "没有选中任何文件！");
    }
    else {
        FileItem * selected_item = (FileItem *)model->itemFromIndex(selected_index);
        Tree::Node * node = selected_item->get_node();
        tree->delete_file(node);
        fflush();
    }    
}
