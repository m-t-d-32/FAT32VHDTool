#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include "file_operator"
#include "tree.h"
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <queue>
#include <QMessageBox>
#include <QByteArray>
#include "file_extracter.h"
#include "file_creator.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("虚拟硬盘");

    model = new QStandardItemModel(ui->treeView);
    ui->treeView->setModel(model);

    direct_operator = nullptr;
    dbr_operator = nullptr;
    tree = nullptr;
}

void MainWindow::resizeEvent(QResizeEvent * event){
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
                  "保存虚拟硬盘文件", nullptr, "虚拟硬盘文件(*.c51);;所有文件(*.*)");
        if (filename.length() > 0){
            direct_operator = new FileOperator(filename);
            dbr_operator = new DBROperator();
            dbr_operator->init_dbr(creator->get_section_size(), creator->get_cluster_size(), creator->get_create_size(), direct_operator);
            QMessageBox::information(nullptr, "成功", "创建文件成功");
            fflush();
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
        }
        set_tree(it, new_item);
    }
}

void MainWindow::on_openFile_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
            "请选择要打开的文件", nullptr, "虚拟硬盘文件(*.c51);;所有文件(*.*)");

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
        direct_operator = new FileOperator(filename);
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
    ui->treeView->setColumnWidth(0, ui->treeView->width() / 2);
    ui->treeView->setColumnWidth(1, ui->treeView->width() / 2);
}


void MainWindow::fflush(){
    delete tree;
    tree = new Tree(dbr_operator);
    model->clear();

    model->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("文件名")<<QStringLiteral("大小"));

    reset_tree_item_width();
    selected_index = model->index(-1, 1);
    //tree->print_tree();

    for (auto it: tree->getRoot()->children){
        FileItem * item = new FileItem(it);
        model->appendRow(item);
        if (is_file(it->file)){
            model->setItem(model->rowCount() - 1, 1, new QStandardItem(to_preferred_size(it->file.size)));
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
            FileExtracter::extract_file(filename, selected_item->get_node(), dbr_operator);
            QMessageBox::information(nullptr, "提取", "提取完毕！");
        }
    }
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    selected_index = index;
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
        fflush();
    }    
}

void MainWindow::on_help_triggered()
{
    QMessageBox::information(nullptr, "关于产品", "本产品是为了宣传编译器框架LYRON构建的，详细请访问https://github.com/llyronx/LYRON");
}
