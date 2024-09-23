#include "logindialog.h"
#include "ui_logindialog.h"

#include <QByteArray>
#include <QToolTip>
#include <QFile>

#include "mainwindow.h"
#include "api.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);

    // Prevent resizing.
    setFixedSize(width(), height());

    QFile token_file("token.txt");
    if (token_file.exists()) {
        if (token_file.open(QFile::OpenModeFlag::ReadOnly)) {
            // TODO: проверять здесь, что сессия с сохранненым токеном не истекла.
            // TODO: при просроченном токене на любой операции с API предупреждать,
            // что нужно перезапустить приложение.
            onLoggedIn(QString(token_file.readAll()).remove('\n').remove('\r'));
            return;
        }
    }

    show();
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
        QString token(result.content);

        QFile token_file("token.txt");
        if (token_file.open(QFile::OpenModeFlag::WriteOnly)) {
            token_file.write(token.toUtf8());
        }

        onLoggedIn(token);
    } else {
        QToolTip::showText(QCursor::pos(), result.content);
    }
}


void LoginDialog::on_helpBtn_clicked()
{
    QToolTip::showText(
        QCursor::pos(),
        "Профиль в формате id1234 или username.\n"
        "Для регистрации введите профиль и нажмите соответствующую кнопку."
        );

}

