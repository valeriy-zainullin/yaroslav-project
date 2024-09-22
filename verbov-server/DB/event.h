#ifndef EVENT_H
#define EVENT_H

#include <cstdint>

#include <QDataStream>
#include <QString>
#include <QStringView>

#include <QtSql/QSqlQuery>

class Event
{
private:
    quint64 id = 0;
public:
    QString name;
    quint64 creator_user_id = 0;
    quint64 timestamp; // UTC+0 unix time when the event will happen
private:
    static const QString table_name;
public:
    bool unpack_from_query(QSqlQuery& query);
    void pack_into_query(QSqlQuery& query, bool fill_id = true) const;

    // Проверяет, что таблица существует. Если нет, создает.
    static bool check_table(QSqlDatabase& db);

    static bool run_tests(QSqlDatabase& test_db);

    static bool fetch_by_id(QSqlDatabase& db, uint64_t id, std::optional<Event>& found_event);
    static bool fetch_all_for_user(QSqlDatabase& db, uint64_t user_id, QVector<Event>& found_events);
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

    friend QDataStream& operator<<(QDataStream& out, const Event& entry);
    friend QDataStream& operator>>(QDataStream& in,  Event& entry);

// ...
public:
    Event();
};

// Serialization and deserialization.
QDataStream& operator<<(QDataStream& out, const Event& entry);
QDataStream& operator>>(QDataStream& in,  Event& entry);

#endif // EVENT_H
