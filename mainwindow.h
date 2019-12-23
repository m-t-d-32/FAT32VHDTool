#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <file_operator.h>
#include <tree.h>
#include <fat32_file_reader.h>
#include <dbr_operator.h>
#include <QStandardItemModel>
#include <file_item.h>
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

    void on_extractButton_clicked();

    void on_treeView_clicked(const QModelIndex &index);

    void on_deleteButton_clicked();

    void on_help_triggered();

    void on_addFileButton_clicked();

    void on_addFolderButton_clicked();
protected:

    virtual void resizeEvent(QResizeEvent * event) override;

private:
    Ui::MainWindow *ui;
    QStandardItemModel * model;

    FileOperator * direct_operator;
    DBROperator * dbr_operator;
    Tree * tree;
    QModelIndex selected_index;

    void set_tree(Tree::Node * root, FileItem * item);

    void reset_tree_item_width();

    void fflush();

};

#endif // MAINWINDOW_H
