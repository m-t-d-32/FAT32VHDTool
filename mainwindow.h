#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "DirectRead.h"
#include "Exception.h"
#include "Tree.h"
#include "FAT32FileReader.h"
#include "DBRReader.h"
#include <QStandardItemModel>
#include "fileitem.h"
#include <QModelIndex>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_newFile_triggered();

    void on_openFile_triggered();

    void setTree(Tree::Node * root, FileItem * item);

    void on_pushButton_clicked();

    void on_treeView_clicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    QStandardItemModel * model;

    DirectRead * directReader;
    DBRReader * fat32Reader;
    FAT32FileReader * fat32FileReader;
    Tree * tree;

    QModelIndex selected_index;
};

#endif // MAINWINDOW_H
