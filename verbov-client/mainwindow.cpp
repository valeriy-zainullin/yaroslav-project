#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QString token, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , token_(std::move(token))
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}
