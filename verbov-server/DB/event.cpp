#include "event.h"

#include <QtSql/QSqlTableModel>
#include <QtSql/QSqlQuery>
#include <QCryptographicHash>
#include <QDateTime>
#include <QSqlError>

#include <optional>

#include "user.h"

const QString Event::table_name = "Events";

#define CHECK(expr) if (!(expr)) { return false; }
bool Event::run_tests(QSqlDatabase& test_db) {
    return true;
}
#undef CHECK

bool Event::unpack_from_query(QSqlQuery& query) {
    bool right_variant = true;
    id = query.value("id").toULongLong(&right_variant);
    if (!right_variant) {
        // Some programming or db error, treat as a failed query.
        return false;
    }

    name = query.value("name").toString();

    creator_user_id = query.value("creator_user_id").toULongLong(&right_variant);
    if (!right_variant) {
        // Some programming or db error, treat as a failed query.
        return false;
    }

    timestamp = query.value("timestamp").toULongLong(&right_variant);
    if (!right_variant) {
        // Some programming or db error, treat as a failed query.
        return false;
    }

    return true;
}

void Event::pack_into_query(QSqlQuery& query, bool fill_id) const {
    if (fill_id) {
        // If we are creating a row, then id is not known, we should not set it.
        query.bindValue(":id", QVariant::fromValue(id));
    }

    query.bindValue(":name", name);
    query.bindValue(":creator_user_id", QVariant::fromValue(creator_user_id));
    query.bindValue(":timestamp", QVariant::fromValue(timestamp));
}

bool Event::check_table(QSqlDatabase& db) {
    // QSqlQuery docs: https://doc.qt.io/qt-6/sql-sqlstatements.html
    QSqlQuery query(db);

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
        "id INTEGER               NOT NULL PRIMARY KEY AUTOINCREMENT CHECK(id >= 1),"
        "name VARCHAR(64)         NOT NULL                           CHECK(LENGTH(name) = 64),"
        "creator_user_id INTEGER  NOT NULL                           CHECK(creator_user_id >= 1),"
        "timestamp INTEGER(8)     NOT NULL                           CHECK(timestamp >= 0),"
        "FOREIGN KEY (creator_user_id) REFERENCES " + User::table_name + "(id) ON DELETE RESTRICT"
    ");");

    if (!query.exec()) {
        // Failed to execute the query.
        qCritical() << query.lastError().text();
        return false;
    }

    return true;
}

bool Event::fetch_by_id(QSqlDatabase& db, uint64_t id, std::optional<Event>& found_session) {
    Event event;
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
        found_session.reset();
        return true;
    }

    if (!event.unpack_from_query(query)) {
        // Failed to unpack. Treat as a failed query.
        return false;
    }

    found_session = std::move(event);

    return true;
}

bool Event::fetch_all_for_user(QSqlDatabase& db, uint64_t user_id, QVector<Event>& found_events) {
    Event event;
    QSqlQuery query(db);

    query.prepare("SELECT * FROM " + table_name + " WHERE creator_user_id = :user_id");
    query.bindValue(":user_id", QVariant::fromValue(user_id));

    if (!query.exec()) {
        // Failed to execute the query.
        qCritical() << query.lastError().text();
        return false;
    }

    found_events.clear();

    if (query.first()) {
        do {
            Event event;

            if (!event.unpack_from_query(query)) {
                // Failed to unpack. Treat as a failed query.
                return false;
            }

            found_events.push_back(std::move(event));
        } while (query.next());
    }

    return true;
}

// ..., аналогично предыдущим моделям

Event::Event() {}
