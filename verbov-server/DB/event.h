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
// ...
public:
    Event();
};

#endif // EVENT_H
