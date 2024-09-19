#include "logindialog.h"
#include "ui_logindialog.h"

#include "mainwindow.h"
#include "tooltip.h"
#include "api.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);

    // Prevent resizing.
    setFixedSize(width(), height());
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::on_regBtn_clicked()
{
    QString vkProfile = ui->vkEdit->text();
    // QString pass = ui->passEdit->text();
    // qInfo() << vkId << " " << pass;

    api::Result result = api::request("register", {"vk_profile"}, {vkProfile});

    // Указали this, потому объект удалится вместе с
    // текущим окном. На самом деле внутри подсказки
    // есть таймер, он вызовет deleteLater у окна.
    // Оно удалится, как только обработка сигнала
    // завершится и управление перейдет в event loop
    // обработки сообщений от ОС о действиях с окном,
    // Такой event loop видно, если писать GUI
    // на winapi в dev-c++..
    // Таким event loop должен быть QCoreApplication::exec.
    // https://doc.qt.io/qt-6/qobject.html#deleteLater
    // https://doc.qt.io/qt-6/qcoreapplication.html#exec
    // https://forum.qt.io/topic/91180/delete-vs-deletelater/2
    new Tooltip(result.message, this);
}

void LoginDialog::onLoggedIn(QString token) {
    mainWnd.emplace();
    mainWnd->show();

    hide();
}


void LoginDialog::on_loginBtn_clicked()
{
    onLoggedIn("we should have token here!");
}

