#include "file_creator.h"
#include "ui_filecreator.h"
#include "defines.h"

FileCreator::FileCreator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::createfile)
{
    ui->setupUi(this);
    ui->spinBox_2->setFocusPolicy(Qt::NoFocus);
    will_accepted = false;
    will_create_size = 256;
    will_cluster_size = 1;
    will_section_size = 512;
}

FileCreator::~FileCreator()
{
    delete ui;
}

void FileCreator::on_horizontalScrollBar_valueChanged(int value)
{
    if (value < EVERY_GB){
        ui->lcdNumber->display(value);
        ui->blockSizeBox->setText("M");
    }
    else {
        ui->lcdNumber->display(value / (float)EVERY_GB);
        ui->blockSizeBox->setText("G");
    }
    will_create_size = value;
}

int FileCreator::get_create_size(){
    return will_create_size;
}

int FileCreator::get_cluster_size(){
    return will_cluster_size;
}

int FileCreator::get_section_size(){
    return will_section_size;
}

void FileCreator::on_spinBox_valueChanged(int arg1)
{
    will_cluster_size = arg1;
}

void FileCreator::on_spinBox_2_valueChanged(int arg1)
{
    will_section_size = arg1;
}

bool FileCreator::get_accepted(){
    return will_accepted;
}

void FileCreator::on_buttonBox_accepted()
{
    will_accepted = true;
}
