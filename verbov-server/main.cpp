#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QtSql/QSqlDatabase>
#include <QTranslator>
#include <QFile>

#include "DB/session.h"
#include "DB/user.h"

#define CHECK(expr) if (!(expr)) { return false; }
static bool check_tables(QSqlDatabase& db) {
    CHECK(User::check_table(db));
    CHECK(Session::check_table(db));

    return true;
}

static bool run_tests() {
    const QString db_path = "test_db.sqlite3";
    if (QFile().exists(db_path)) {
        // Удалим БД с прошлого раза, чтобы не ловить ошибки из-за незавершившихся операций.
        CHECK(QFile().remove(db_path));
    }

    // https://stackoverflow.com/a/27844918
    QSqlDatabase test_db = QSqlDatabase::addDatabase("QSQLITE");
    test_db.setDatabaseName(db_path);
    CHECK(test_db.open());

    CHECK(check_tables(test_db));

    CHECK(User::run_tests(test_db));
    CHECK(Session::run_tests(test_db));

    return true;
}
#undef CHECK

static bool run_server() {
    return true;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qInfo() << "run_tests: " << run_tests() << '\n';

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "verbov-server_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
