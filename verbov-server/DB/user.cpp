#include "user.h"

#include <QtSql/QSqlTableModel>
#include <QtSql/QSqlQuery>
#include <QCryptographicHash>
#include <QSqlError>

#include <optional>

const QString User::table_name = "Users";

const QString password_salt = "#7923sdjjfkddrjkhjfjdflksjglmxc.,vmlk;g";
// Если БД утечет, подобрать именно пароли не получится путем подбора строки,
//   у которой такой же хеш. Вот бы еще код не утек.

#define CHECK(expr) if (!(expr)) { return false; }
// Не используется, если включен NDEBUG.
bool User::run_tests(QSqlDatabase& test_db) {
    User user1;
    user1.first_name = "Ярослав";
    user1.last_name = "Вербов";
    user1.email = "verbov.iaiu@phystech.edu";
    user1.set_password(QString("234"));

    // Test password.
    CHECK(user1.check_password(QString("234")));
    CHECK(!user1.check_password(QString("235")));

    // Test CRUD.

    // Create.
    CHECK(user1.create(test_db));
    // Create should fail, because such row already exists (primary key and unique constraints will fail).
    CHECK(!user1.create(test_db));

    // Read.
    std::optional<User> user1_from_db;
    CHECK(fetch_by_id(test_db, user1.id, user1_from_db));
    CHECK(user1_from_db.has_value());
    CHECK(user1_from_db.value() == user1);

    CHECK(fetch_by_email(test_db, QString("verbov.iaiu@phystech.edu"), user1_from_db));
    CHECK(user1_from_db.has_value());
    CHECK(user1_from_db.value() == user1);

    // Check non-existent user.
    std::optional<User> nonexistent_user_from_db;
    CHECK(fetch_by_id(test_db, 0, nonexistent_user_from_db))
    CHECK(!nonexistent_user_from_db.has_value());
    CHECK(fetch_by_email(test_db, QString("hehe@haha?"), nonexistent_user_from_db))
    CHECK(!nonexistent_user_from_db.has_value());

    // Update.
    // Should be able to update.
    // Обязательно все поля меняем, чтобы протестировать update.
    user1.first_name = user1.first_name.replace("с", "c"); // Русскую "с" заменил на латинскую, подмену не заметят)
    user1.last_name = user1.last_name.replace("о", "o"); // Русскую "о" заменил на латинскую, подмену не заметят)
    user1.email = "hehe@haha?";
    user1.set_password(QString("heheHaHa?"));
    CHECK(user1.update(test_db));
    CHECK(fetch_by_id(test_db, user1.id, user1_from_db));
    CHECK(user1_from_db.value() == user1);

    // Delete.
    CHECK(user1.drop(test_db));
    CHECK(fetch_by_email(test_db, user1.email, nonexistent_user_from_db)); // ID was set to zero, so let's use email.
    CHECK(!nonexistent_user_from_db.has_value());

    return true;
}
#undef CHECK

static QString hash_password(const QStringView password) {
    // QCryptographicHash docs: https://doc.qt.io/qt-5/qcryptographichash.html
    QCryptographicHash hash(QCryptographicHash::Algorithm::Sha3_256);
    hash.addData(password.toUtf8());
    hash.addData(password_salt.toUtf8());
    return hash.result().toHex();
}

// User
//   uint64_t id;
//   QString first_name;
//   QString last_name;
//   QString email;
//   QString password_hash;
bool User::unpack_from_query(QSqlQuery& query) {
    // Unpack a returned user. First row is extracted.
    bool right_variant = true;
    id = query.value("id").toULongLong(&right_variant);
    if (!right_variant) {
        // Some programming or db error, treat as a failed query.
        return false;
    }

    first_name = query.value("first_name").toString();
    last_name = query.value("last_name").toString();
    email = query.value("email").toString();
    password_hash = query.value("password_hash").toString();

    return true;
}
void User::pack_into_query(QSqlQuery& query, bool fill_id) const {
    if (fill_id) {
        // If we are creating a row, then id is not known, we should not set it.
        query.bindValue(":id", QVariant::fromValue(id));
    }
    query.bindValue(":first_name", first_name);
    query.bindValue(":last_name", last_name);
    query.bindValue(":email", email);
    query.bindValue(":password_hash", password_hash);
}

bool User::check_table(QSqlDatabase& db) {
    // QSqlQuery docs: https://doc.qt.io/qt-6/sql-sqlstatements.html
    QSqlQuery query(db);

    // uint64_t id;
    // QString first_name;
    // QString last_name;
    // QString email;
    // QString password_hash;
    // SQL LIKE operator. https://www.w3schools.com/sql/sql_like.asp
    // Turns out ids in sqlite start from 1. That's great.
    //   https://stackoverflow.com/questions/692856/set-start-value-for-autoincrement-in-sqlite
    //   The initial value for SQLITE_SEQUENCE.seq must be null or 0. But from tests seems like ids
    //   start from 1.
    //   https://stackoverflow.com/a/9053277
    // We have to write NOT NULL at PRIMARY KEYS of not integer type for sqlite due to a bug.
    //   https://stackoverflow.com/a/64778551
    query.prepare(
        "CREATE TABLE IF NOT EXISTS " + table_name + "("
        "id INTEGER                NOT NULL PRIMARY KEY AUTOINCREMENT CHECK(id >= 1),"
        "first_name VARCHAR(64)    NOT NULL                           CHECK(first_name != ''),"
        "last_name VARCHAR(64)     NOT NULL                           CHECK(last_name != ''),"
        "email VARCHAR(64)         NOT NULL UNIQUE                    CHECK(email LIKE '%_@_%'),"
        "password_hash VARCHAR(64) NOT NULL                           CHECK(LENGTH(password_hash) = 64)"
    ");");

    if (!query.exec()) {
        // Failed to execute the query.
        qCritical() << query.lastError().text();
        return false;
    }

    return true;
}


bool User::fetch_by_id(QSqlDatabase& db, uint64_t id, std::optional<User>& found_user) {
    User user;
    QSqlQuery query(db);

    query.prepare("SELECT * FROM " + table_name + " WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(id));

    if (!query.exec()) {
        // Failed to execute the query.
        qCritical() << query.lastError().text();
        return false;
    }

    if (!query.first()) {
        // Haven't found anything. But the query is successful.
        found_user.reset();
        return true;
    }

    if (!user.unpack_from_query(query)) {
        // Failed to unpack. Treat as a failed query.
        return false;
    }

    found_user = std::move(user);

    return true;
}
bool User::fetch_by_email(QSqlDatabase& db, const QStringView email, std::optional<User>& found_user) {
    // Almost ctrl-c + ctrl-v from code for retrieval by id.
    // Query one row by an unique column.

    User user;
    QSqlQuery query(db);

    query.prepare("SELECT * FROM " + table_name + " WHERE email = :email");
    query.bindValue(":email", email.toString());

    if (!query.exec()) {
        // Failed to execute the query.
        qCritical() << query.lastError().text();
        return false;
    }

    if (!query.first()) {
        // Haven't found anything. But the query is successful.
        found_user.reset();
        return true;
    }

    if (!user.unpack_from_query(query)) {
        // Failed to unpack. Treat as a failed query.
        return false;
    }

    found_user = std::move(user);

    return true;
}

bool User::create(QSqlDatabase& db) {
    QSqlQuery query(db);

    query.prepare(
        "INSERT INTO " + table_name + "(first_name, last_name, email, password_hash)"
        " VALUES (:first_name, :last_name, :email, :password_hash)"
    );
    pack_into_query(query, false);

    if (!query.exec()) {
        // Failed to execute the query.
        qCritical() << query.lastError().text();
        return false;
    }

    bool right_variant = false;
    id = query.lastInsertId().toULongLong(&right_variant);
    if (!right_variant) {
        // The only other way is to throw an exception..
        // But then we have to create a custom exception.
        abort();
    }

    return true;
}


bool User::update(QSqlDatabase& db) {
    QSqlQuery query(db);

    query.prepare(
        "UPDATE " + table_name + " " +
        "SET first_name = :first_name, last_name = :last_name, email = :email, password_hash = :password_hash "
        "WHERE id = :id"
    );
    pack_into_query(query);

    if (!query.exec()) {
        // Failed to execute the query.
        qCritical() << query.lastError().text();
        return false;
    }

    return true;
}

bool User::drop(QSqlDatabase& db) {
    QSqlQuery query(db);

    query.prepare("DELETE FROM " + table_name + " WHERE id = :id");
    query.bindValue(":id", QVariant::fromValue(id));

    if (!query.exec()) {
        // Failed to execute the query.
        qCritical() << query.lastError().text();
        return false;
    }

    id = 0; // id == 0 -- строка еще не в БД.

    return true;
}

void User::set_password(const QStringView password) {
    password_hash = hash_password(password);
}

bool User::check_password(const QStringView input_password) const {
    return password_hash == hash_password(input_password);
}


User::User() {}
