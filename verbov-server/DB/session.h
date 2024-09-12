#ifndef SESSION_H
#define SESSION_H

#include <cstdint>

#include <QString>
#include <QStringView>

#include <QtSql/QSqlQuery>

class Session
{
private:
    uint64_t id = 0;
public:
    uint64_t user_id = 0;

    QString token;
    uint64_t start_time = 0;
private:
    static const QString table_name;
    static const uint64_t mav_duration_sec = 5 * 24 * 60 * 60;
public:
    // Public static methods.

    bool unpack_from_query(QSqlQuery& query);
    void pack_into_query(QSqlQuery& query, bool fill_id = true) const;

    // Проверяет, что таблица существует. Если нет, создает.
    static bool check_table(QSqlDatabase& db);

    static bool run_tests(QSqlDatabase& test_db);

    static bool fetch_by_id(QSqlDatabase& db, uint64_t id, std::optional<Session>& found_session);
    static bool fetch_by_token(QSqlDatabase& db, const QStringView token, std::optional<Session>& found_session);
public:
    // Public plain methods.

    uint64_t get_id() const { return id; }

    // Если объекта нет в БД, то update и drop могут вернуть успех, ничего не сделав.
    // Обновить или удалить 0 строк вполне нормально по мнению запроса SQL. Можно проверять
    // что реально какие-то операции были типо RETURNING id добавить какой-нибудь в запросы.
    // Но пока так.

    bool create(QSqlDatabase& db);
    bool update(QSqlDatabase& db);
    bool drop(QSqlDatabase& db);

    bool generate_token(QSqlDatabase& db);
    void set_time_started();
    bool is_expired();

    Session();

    bool operator==(const Session& other) const = default;
};

#endif // SESSION_H