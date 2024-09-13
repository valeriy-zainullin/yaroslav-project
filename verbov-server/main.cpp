#include <iostream>

#include <QtSql/QSqlDatabase>
#include <QHttpServer>
#include <QFile>

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(__unix__)
#include <signal.h>
#endif

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

bool exiting = false;
static void run_server() {
    // Register signal handlers.
#if defined(_WIN32)
    // Building for windows.
    // https://stackoverflow.com/questions/7581343/how-to-catch-ctrlc-on-windows-and-linux-with-qt
    // https://learn.microsoft.com/ru-ru/windows/console/setconsolectrlhandler
    SetConsoleCtrlHandler([](DWORD ctrlType) {
        if (ctrlType == CTRL_C_EVENT) {
            exiting = true;
        }
        return TRUE; // Consume the signal.
    }, FALSE);
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    struct sigaction action = {};
    action.sa_handler = [](int dummy) {
        exiting = true;
    };
    sigaction(SIGINT, &action, nullptr);
#endif

    // https://www.qt.io/blog/2019/02/01/qhttpserver-routing-api

    QHttpServer server;
    server.route("/register", []() {
        return "Hello, you are about to be registered.";
    });
    server.listen(QHostAddress("127.0.0.1"), 8080);

    qInfo() << "Server is up on 127.0.0.1:8080";
    qInfo() << "Type ctrl-c to exit.";

    while (!exiting);
}

int main(int argc, char *argv[])
{
    qInfo() << "run_tests: " << run_tests();

    run_server();

    return 0;
}
