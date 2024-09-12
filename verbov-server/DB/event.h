#ifndef EVENT_H
#define EVENT_H

#include <cstdint>

#include <QString>
#include <QStringView>

#include <QtSql/QSqlQuery>

class Event
{
private:
    uint64_t id = 0;
public:
    QString name;
    uint64_t creator_user_id = 0;
    uint64_t timestamp; // UTC+0 unix time when the event will happen
private:
    static const QString table_name;
public:
    bool unpack_from_query(QSqlQuery& query);
    void pack_into_query(QSqlQuery& query, bool fill_id = true) const;

    // Проверяет, что таблица существует. Если нет, создает.
    static bool check_table(QSqlDatabase& db);

    static bool run_tests(QSqlDatabase& test_db);

    static bool fetch_by_id(QSqlDatabase& db, uint64_t id, std::optional<Event>& found_event);
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

    bool operator==(const Event& other) const = default;
// ...
public:
    Event();
};

#endif // EVENT_H