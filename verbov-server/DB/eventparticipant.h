#ifndef EVENTPARTICIPANT_H
#define EVENTPARTICIPANT_H

#include <cstdint>

#include <QString>
#include <QStringView>

#include <QtSql/QSqlQuery>

class EventParticipant
{
private:
    uint64_t id = 0;
public:
    uint64_t event_id = 0;
    uint64_t user_id = 0;

    bool can_invite = false; // Can invite more people onto the event.

    uint64_t registered_time = 0; // UTC+0 unix time when the participant was registered onto the event.
    // It is used to not send excessive notification. E.g. user registers the day before the event, we shouldn't notify him twice like
    // "hey, it's less than a week" before the event and then "hey, it's less than a day before the event".
    // Also this only works if server is up at least every day, so that notification duties don't pile up.

    // 0 -- never notified
    // 1 -- notified week before the event (if between event creation and it's occurence there is a week)
    // 2 -- notified 3 days before the event
    // 3 -- notified day before the event
    // 4 -- notified 6 hours before the event
    // 5 -- notified an hour before the event
    // 6 -- notified 20 minutes before the event.
    uint64_t last_notification = 0;
public:
    EventParticipant();
};

#endif // EVENTPARTICIPANT_H
