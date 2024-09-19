#ifndef USER_H
#define USER_H

#include <cstdint>

#include <QString>
#include <QStringView>

#include <QtSql/QSqlQuery>

class User
{
private:
    qint64 vk_id;
public:
    QString first_name;
    QString last_name;
    bool reg_confirmed = false;
    uint32_t reg_code = 0;

    QString password_hash;
public:
    friend class Session; // Needs table_name from here.
    static const QString table_name;
public:
    // Public static methods.

    bool unpack_from_query(QSqlQuery& query);
    void pack_into_query(QSqlQuery& query) const;

    // Проверяет, что таблица существует. Если нет, создает.
    static bool check_table(QSqlDatabase& db);

    static bool run_tests(QSqlDatabase& test_db);

    static bool fetch_by_vk_id(QSqlDatabase& db, qint64 vk_id, std::optional<User>& found_user);
public:
    // Public plain methods.

    uint64_t get_vk_id() const { return vk_id; }

    // Если объекта нет в БД, то update и drop могут вернуть успех, ничего не сделав.
    // Обновить или удалить 0 строк вполне нормально по мнению запроса SQL. Можно проверять
    // что реально какие-то операции были типо RETURNING id добавить какой-нибудь в запросы.
    // Но пока так.

    bool create(QSqlDatabase& db);
    bool update(QSqlDatabase& db);
    bool drop(QSqlDatabase& db);

    void set_password(const QStringView password);
    bool check_password(const QStringView input_password) const;

    User(qint64 vk_id)
        : vk_id(vk_id) {}

    bool operator==(const User& other) const = default;

private:
    User();
};

#endif // USER_H
