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

    onLoggedIn("478d560b326501d4e07248e95ee5c5037fd4ccc526aafbc793dd20a7319e88f1");
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

    QToolTip::showText(QCursor::pos(), result.content);
}

void LoginDialog::onLoggedIn(QString token) {
    mainWnd.emplace(token);
    mainWnd->show();

    hide();
}


void LoginDialog::on_loginBtn_clicked()
{
    QString vkProfile = ui->vkEdit->text();
    QString pass = ui->passEdit->text();

    api::Result result = api::request("login", {"vk_profile", "password"}, {vkProfile, pass});

    if (result.success) {
        onLoggedIn(result.content);
    } else {
        QToolTip::showText(QCursor::pos(), result.content);
    }
}

