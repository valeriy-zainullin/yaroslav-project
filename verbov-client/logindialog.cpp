#include "logindialog.h"
#include "ui_logindialog.h"

#include <QToolTip>

#include "mainwindow.h"
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

    QToolTip::showText(QCursor::pos(), result.message);
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

