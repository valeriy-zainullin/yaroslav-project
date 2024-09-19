#ifndef API_H
#define API_H

#include <QString>

namespace api {
    struct Result {
        bool success = false;
        QString message;
    };

    Result request(const QString& method, const QVector<QString>& args, const QVector<QString>& values, const QString& token = "");
}

#endif // API_H
