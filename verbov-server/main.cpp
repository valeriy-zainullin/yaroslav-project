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

    session.user_id = user.get_vk_id();

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

static QString make_confirmation_link(const User& user) {
    return "http://127.0.0.1:8080/reg_confirm?vk_id=" +
           QString::number(user.get_vk_id()) + "&reg_code=" +
           QString::number(user.reg_code);
}

static QString generate_password() {
    struct CharSeg {
        char start;
        size_t length;
    };
    static constexpr CharSeg alphabet[] = {
        {'a', 'z'-'a'+1},
        {'A', 'Z'-'A'+1},
        {'0', 10},
    };
    static constexpr size_t alphabet_len = ('z'-'a'+1)+('Z'-'A'+1)+10;
    static constexpr char specials[] = {'.', ',', ';', '&', '%', '!'};

    auto generator = std::mt19937(std::random_device()());
    static constexpr size_t length = 10;

    QString result;
    for (size_t i = 0; i < length; ++i) {
        bool is_special = generator() % 4 == 0;
        char added_char = 0;
        if (is_special) {
            added_char = specials[generator() % (sizeof(specials) / sizeof(char))];
        } else {
            size_t index = generator() % alphabet_len;
            for (size_t seg = 0; seg < sizeof(alphabet) / sizeof(*alphabet); ++seg) {
                if (index >= alphabet[seg].length) {
                    index -= alphabet[seg].length;
                } else {
                    added_char = alphabet[seg].start + index;
                    break;
                }
            }
        }
        result.append(added_char);
    }
    return result;
}

static QString basic_html(const QString& text) {
    return "<!DOCTYPE html><html><meta charset=\"utf-8\">"
           "<title>Приложение \"Календарь\"></title><head></head><body>" +
           text +
           "</body></html>";
}

static int run_server(QCoreApplication& app) {
    const QString db_path = "db.sqlite3";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(db_path);
    if (!db.open()) {
        return -2;
    }

    if (!check_tables(db)) {
        qFatal("Failed to prepare tables");
        return -3;
    }

    QHttpServer server;
    // https://doc.qt.io/qt-6/qhttpserver.html#route
    server.route("/register", [&db](const QHttpServerRequest& request) {
        // POST как бы лучше для этого, но ладно..
        QString vk_profile = request.query().queryItemValue("vk_profile");

        int vk_error_code = 0;
        QString vk_error_msg;
        qint64 vk_id = 0;
        QString first_name;
        QString last_name;
        if (!vk::get_user(vk_profile, first_name, last_name, vk_id, vk_error_code, vk_error_msg)) {
            // 18  Страница удалена или заблокирована.
            // 30  Профиль является приватным
            // 113 Неверный идентификатор пользователя.
            if (vk_error_code == 18 || vk_error_code == 30 || vk_error_code == 113) {
                return QHttpServerResponse(
                    "Пользователь VK не найден.",
                    QHttpServerResponse::StatusCode::NotFound
                    );
            }

            return QHttpServerResponse(
                "Ошибка при взаимодействии с VK API.",
                QHttpServerResponse::StatusCode::InternalServerError
                );
        }

        std::optional<User> maybe_user;
        if (!User::fetch_by_vk_id(db, vk_id, maybe_user)) {
            return QHttpServerResponse(
                "Внутренняя ошибка (1)",
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
                        maybe_user->get_vk_id(),
                        "Добро пожаловать в приложение \"календарь\".\nСсылка для подтверждения вашей регистрации: "
                            + make_confirmation_link(*maybe_user),
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

            User user(vk_id);
            user.first_name = first_name;
            user.last_name = last_name;
            user.set_password(password);
            // Генерируем шестизначный код подтверждения.
            user.reg_code = std::mt19937(std::random_device()())() % 1000000;
            user.reg_confirmed = false;

            if (!user.create(db)) {
                return QHttpServerResponse(
                    "Внутренняя ошибка (2)",
                    QHttpServerResponse::StatusCode::InternalServerError
                    );
            }

            if (!vk::send_message(
                    user.get_vk_id(),
                    "Добро пожаловать в приложение \"календарь\". \nВаш пароль: " + password + "\nСсылка для подтверждения вашей регистрации: " +
                        make_confirmation_link(*maybe_user),
                    0,
                    vk_error_code,
                    vk_error_msg
                )) {
                // 901 Нельзя отправлять сообщения для пользователей без разрешения
                if (vk_error_code == 901) {
                    return QHttpServerResponse(
                        "Проверьте, что разрешены сообщения от группы.",
                        QHttpServerResponse::StatusCode::NotFound
                        );
                }

                qInfo() << vk_error_code;

                return QHttpServerResponse(
                    "Ошибка при взаимодействии с VK API.",
                    QHttpServerResponse::StatusCode::InternalServerError
                    );
            }

            return QHttpServerResponse(
                "Код подтверждения выслан, проверьте сообщения.",
                QHttpServerResponse::StatusCode::Ok
                );
        }
    });
    // Возвращает сообщение для пользователя, которое он увидит в браузере.
    server.route("/reg_confirm", [&db](const QHttpServerRequest& request) {
        // POST как бы лучше для этого, но ладно..
        QString vk_id_str = request.query().queryItemValue("vk_id");

        bool is_integer = false;
        qint64 vk_id = vk_id_str.toULongLong(&is_integer);
        if (!is_integer) {
            return QHttpServerResponse(
                basic_html("Недопустимый id пользователя."),
                QHttpServerResponse::StatusCode::NotFound
                );
        }

        QString reg_code_str = request.query().queryItemValue("reg_code");

        is_integer = false;
        qint64 reg_code = reg_code_str.toULongLong(&is_integer);
        if (!is_integer) {
            return QHttpServerResponse(
                basic_html("Недопустимый код подтверждения."),
                QHttpServerResponse::StatusCode::NotFound
                );
        }

        std::optional<User> maybe_user;
        if (!User::fetch_by_vk_id(db, vk_id, maybe_user)) {
            return QHttpServerResponse(
                basic_html("Внутренняя ошибка (1)."),
                QHttpServerResponse::StatusCode::InternalServerError
                );
        }
        if (!maybe_user.has_value() || maybe_user->reg_code != reg_code || maybe_user->reg_confirmed) {
            return QHttpServerResponse(
                basic_html("Код подтверждения неверен."),
                QHttpServerResponse::StatusCode::NotFound
                );
        }

        maybe_user->reg_confirmed = true;

        if (!maybe_user->update(db)) {
            return QHttpServerResponse(
                basic_html("Внутренняя ошибка (2)."),
                QHttpServerResponse::StatusCode::InternalServerError
                );
        }

        return QHttpServerResponse(
            basic_html("Вы подтвердили аккаунт. Теперь смело возвращайтесь в приложение."),
            QHttpServerResponse::StatusCode::Accepted
        );
    });
    // Возвращает токен или сообщение об ошибке, которое нужно отобразить.
    server.route("/login", [&db](const QHttpServerRequest& request) {
        // POST как бы лучше для этого, но ладно..
        QString vk_profile = request.query().queryItemValue("vk_profile");

        int vk_error_code = 0;
        QString vk_error_msg;
        qint64 vk_id = 0;
        QString first_name;
        QString last_name;
        if (!vk::get_user(vk_profile, first_name, last_name, vk_id, vk_error_code, vk_error_msg)) {
            // 18  Страница удалена или заблокирована.
            // 30  Профиль является приватным
            // 113 Неверный идентификатор пользователя.
            if (vk_error_code == 18 || vk_error_code == 30 || vk_error_code == 113) {
                return QHttpServerResponse(
                    "Пользователь VK не найден.",
                    QHttpServerResponse::StatusCode::NotFound
                    );
            }

            return QHttpServerResponse(
                "Ошибка при взаимодействии с VK API.",
                QHttpServerResponse::StatusCode::InternalServerError
                );
        }

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

    // int error_code = 0;
    // QString error_msg;
    // vk::send_message("verbovyar21", QString::fromUtf8("Ты зареган!"), 0, error_code, error_msg);

    return run_server(app);
}
