#include <QCoreApplication>
#include <QtSql/QSqlDatabase>
#include <QHttpServer>
#include <QFile>

#include "DB/session.h"
#include "DB/user.h"

#include "vk.h"

#define CHECK(expr) if (!(expr)) { return false; }
static bool check_tables(QSqlDatabase& db) {
    CHECK(User::check_table(db));
    CHECK(Session::check_table(db));

    return true;
}

static bool run_tests() {
    const QString db_path = "test_db.sqlite3";
    if (QFile().exists(db_path)) {
        // Удалим БД с прошлого раза, чтобы не ловить ошибки из-за незавершившихся
        // полностью тестов.
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

bool exiting = false;
static int run_server(QCoreApplication& app) {
    QHttpServer server;
    server.route("/register", []() {
        return "Hello, you are about to be registered.";
    });

    uint16_t port = server.listen(QHostAddress("127.0.0.1"), 8080);
    if (port == 0) {
        qInfo() << "failed to listen on port";
        return -1;
    }

    // Maybe graceful shutdown later..
    qInfo() << "Server is up on 127.0.0.1:8080";
    return app.exec();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qInfo() << "run_tests: " << run_tests();

    int error_code = 0;
    QString error_msg;
    vk::send_message("id574927920", "Сообщение", error_code, error_msg);

    return run_server(app);
}
