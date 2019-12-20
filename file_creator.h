#ifndef CREATEFILE_H
#define CREATEFILE_H

#include <QDialog>

namespace Ui {
class createfile;
}

class FileCreator : public QDialog
{
    Q_OBJECT

public:
    explicit FileCreator(QWidget *parent = nullptr);
    ~FileCreator();

    int get_create_size();
    int get_cluster_size();
    int get_section_size();
    bool get_accepted();

private:

    int will_create_size, will_cluster_size, will_section_size;
    bool will_accepted;

private slots:
    void on_horizontalScrollBar_valueChanged(int value);

    void on_spinBox_valueChanged(int arg1);

    void on_spinBox_2_valueChanged(int arg1);

    void on_buttonBox_accepted();

private:
    Ui::createfile *ui;
};

#endif // CREATEFILE_H
