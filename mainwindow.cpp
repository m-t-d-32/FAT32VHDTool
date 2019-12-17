#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDebug>
#include "DirectRead.h"
#include "Tree.h"
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <queue>
#include <QMessageBox>
#include <QByteArray>
#include "Extract.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("虚拟硬盘");

    model = new QStandardItemModel(ui->treeView);
    ui->treeView->setModel(model);
    model->setHorizontalHeaderLabels(QStringList()<<QStringLiteral("文件名")<<QStringLiteral("完整名"));
    selected_index = model->index(-1, 1);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_newFile_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this,
              "保存虚拟硬盘文件", nullptr, "虚拟硬盘文件(*.c51);;所有文件(*.*)");
    //qDebug() << filename << "\n";
}

void MainWindow::setTree(Tree::Node * root, FileItem * item){
    for (auto it: root->children){
        FileItem * newItem = new FileItem(it);
        item->appendRow(newItem);
        setTree(it, newItem);
    }
}

void MainWindow::on_openFile_triggered()
{
    QString filename = QFileDialog::getOpenFileName(this,
            "请选择要打开的文件", NULL, "虚拟硬盘文件(*.c51);;所有文件(*.*)");
    directReader = new DirectRead(filename.toLatin1().data());
    fat32Reader = new DBRReader(directReader);
    fat32Reader->verify_fat32();
    Tree * tree = new Tree(fat32Reader);
    tree->print_tree();

    for (auto it: tree->getRoot()->children){
        FileItem * item = new FileItem(it);
        model->appendRow(item);
        setTree(it, item);
    }
}

void MainWindow::on_pushButton_clicked()
{
    if (selected_index.row() < 0){
        QMessageBox::information(NULL, "无法提取", "没有选中任何文件！");
    }
    else {
        FileItem * selectedItem = (FileItem *)model->itemFromIndex(selected_index);
        QString defaultName = selectedItem->getNode()->file.long_filename;
        QString filename = QFileDialog::getSaveFileName(this,
            "请选择要保存的路径", defaultName, "所有文件(*)");
        QByteArray array = filename.toLatin1();
        extractFile(array.data(), selectedItem->getNode(), fat32Reader);
    }
}

void MainWindow::on_treeView_clicked(const QModelIndex &index)
{
    selected_index = index;
}
