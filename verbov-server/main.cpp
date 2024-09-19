#include <random>

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

static bool create_session(QSqlDatabase& db, const User& user, std::optional<Session>& new_session) {
    Session session;

    session.user_id = user.get_id();

    if (!session.generate_token(db)) {
        return false;
    }

    session.set_time_started();

    if (!session.create(db)) {
        return false;
    }
    new_session = std::move(session);

    return true;
}

static QString generate_password() {
    return "";
}

bool exiting = false;
static int run_server(QCoreApplication& app) {
    const QString db_path = "db.sqlite3";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(db_path);
    if (!db.open()) {
        return -2;
    }

    QHttpServer server;
    // https://doc.qt.io/qt-6/qhttpserver.html#route
    server.route("/register", [&db](const QHttpServerRequest& request) {
        // POST как бы лучше для этого, но ладно..
        QString vk_id = request.query().queryItemValue("vk_id");

        std::optional<User> maybe_user;
        if (!User::fetch_by_vk_id(db, vk_id, maybe_user)) {
            return QHttpServerResponse(
                "Внутренняя ошибка",
                QHttpServerResponse::StatusCode::InternalServerError
            );
        }
        if (maybe_user.has_value()) {
            // Такой пользователь уже регистрировался.
            // Он мог уже подтвердить аккаунт. Или еще не подтвердить.
            // Если он не подтвердил, то перевышлем ссылку на подтверждение.
            // Если подтвердил, то скажем, что пользователь уже существует и
            // предложим восстановить пароль, если надо.
            if (maybe_user->reg_confirmed) {
                return QHttpServerResponse(
                    "Пользователь уже существует. Попробуйте восстановить пароль.",
                    QHttpServerResponse::StatusCode::Conflict
                );
            } else {
                int vk_error_code = 0;
                QString vk_error_msg;
                if (vk::send_message(
                        maybe_user->vk_id,
                        "Добро пожаловать в приложение \"календарь\". Ссылка для подтверждение вашей регистрации: " +
                            QString(""), // + QString(make_confirmation_link(user)),
                        0,
                        vk_error_code,
                        vk_error_msg
                        )) {
                    return QHttpServerResponse(
                        "Ссылка для подтверждения аккаунта выслана повторно.",
                        QHttpServerResponse::StatusCode::Accepted
                        );
                } else {
                    return QHttpServerResponse(
                        "Ошибка при отправке сообщения. Проверьте, что сообщения от группы разрешены.",
                        QHttpServerResponse::StatusCode::InternalServerError
                    );
                }
            }
        } else {
            QString password = generate_password();

            User user;

            int vk_error_code = 0;
            QString vk_error_msg;
            qint64 user_id; // just to feed the value somewhere
            if (!vk::get_user(vk_id, user.first_name, user.last_name, user_id, vk_error_code, vk_error_msg)) {
                // 18  Страница удалена или заблокирована.
                // 30  Профиль является приватным
                // 113 Неверный идентификатор пользователя.
                if (vk_error_code == 18 || vk_error_code == 30 || vk_error_code == 113) {
                    return QHttpServerResponse(
                        "Пользователь VK не найден",
                        QHttpServerResponse::StatusCode::NotFound
                        );
                }

                return QHttpServerResponse(
                    "Ошибка при взаимодействии с VK API",
                    QHttpServerResponse::StatusCode::InternalServerError
                    );
            }

            user.vk_id = vk_id;
            user.set_password(password);
            // Генерируем шестизначный код подтверждения.
            user.reg_code = std::mt19937(std::random_device()())() % 1000000;
            user.reg_confirmed = false;

            if (!user.create(db)) {
                return QHttpServerResponse(
                    "Внутренняя ошибка",
                    QHttpServerResponse::StatusCode::InternalServerError
                    );
            }

            return QHttpServerResponse(
                "Код подтверждения выслан, проверьте сообщения.",
                QHttpServerResponse::StatusCode::Ok
                );
        }
    });
    // Возвращает токен или сообщение об ошибке, которое нужно отобразить.
    server.route("/login", [&db](const QHttpServerRequest& request) {
        // POST как бы лучше для этого, но ладно..
        QString vk_id = request.query().queryItemValue("vk_id");
        QString password = request.query().queryItemValue("password");

        std::optional<User> maybe_user;

        if (User::fetch_by_vk_id(db, vk_id, maybe_user) && maybe_user->check_password(password)) {
            User user = std::move(maybe_user.value());
            std::optional<Session> maybe_session;

            if (create_session(db, user, maybe_session)) {
                assert(maybe_session.has_value());

                const QString& token = maybe_session->token;
                return QHttpServerResponse(token);
            }
        }

        return QHttpServerResponse(
            "Не удалось войти, проверьте vk_id и пароль",
            QHttpServerResponse::StatusCode::NotFound
        );
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
    vk::send_message("verbovyar21", QString::fromUtf8("Ты зареган!"), 0, error_code, error_msg);

    return run_server(app);
}
