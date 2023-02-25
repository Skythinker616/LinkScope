#include "aboutwindow.h"
#include "ui_aboutwindow.h"

AboutWindow::AboutWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutWindow)
{
    ui->setupUi(this);
    setWindowTitle("LinkScope - About");
}

AboutWindow::~AboutWindow()
{
    delete ui;
}

void AboutWindow::on_btn_back_clicked()
{
    accept();
}
