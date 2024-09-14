#include "vk.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QVector>

// Group token. Should have messages permission.
static const QString vk_group_token = "vk1.a.aUOQhDoQOBcr_FvQp77g9HhCi3PCm9LygaY1b275QKtVwXUXWAI5lMw6-YijCsj_VXwfPwmnYZKy8drof1jOf7AQM5K-mvfpOv-KG26WkCXvkMUA80asrezEtozqFi_FZUZ_KddK-NrKtJNXIlkBffgBcARP_sjn8a2L2-Gug6rqeeUeuk0oQ94f25ecztXerXp4-QxYZ5n1gqT6irtFsQ";

static QJsonDocument do_vkapi_request(QString method, const QVector<QString>& args, const QVector<QString>& values) {
    // https://dev.vk.com/ru/api/api-requests
    // https://stackoverflow.com/questions/46943134/how-do-i-write-a-qt-http-get-request

    // Не выделяем на стеке, т.к. объект может быть большой.
    // Выделяем на куче, даем владеть умному указателю, который сам освободит.
    QScopedPointer<QNetworkAccessManager> netmanager(new QNetworkAccessManager());

    // Этот выделяют на стеке. Видимо, он не такой уж и большой.
    // Заходим внутрь и видим, что внутри много методов и лишь один
    // указатель на какой-то другой объект. Уж 8 байт можно на стеке :)
    QNetworkRequest request;

    // https://forum.qt.io/topic/137547/sending-parameters-by-get-method-to-rest-api/6
    QUrl url("https://api.vk.ru/method/" + method);
    QUrlQuery query;
    query.addQueryItem("v", "5.199"); // API version, developed for version 5.199.
    query.addQueryItem("access_token", vk_group_token);
    qInfo() << "args = " << args;
    qInfo() << "values = " << values;
    Q_ASSERT(args.size() == values.size());
    for (size_t i = 0; i < args.size(); ++i) {
        query.addQueryItem(args[i], values[i]);
    }
    url.setQuery(query);

    request.setUrl(url);

    // Родителем этого объекта должен быть QNetworkManager, по идее, потому
    // с ним освободиться. Не мы выделяли, не мы освобождаем.
    QNetworkReply* reply = netmanager->get(request);

    // Ничего по сути не хранит, просто QObject с методами, которые ждут
    // события -- завершения запроса в интернет.
    QEventLoop loop;
    QObject::connect(netmanager.get(), &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray result = reply->read(20480);
    qInfo() << result;
    QJsonDocument json = QJsonDocument::fromJson(result);

    // Похоже, что у vk api все коды ошибок положительные. Судя по
    // https://dev.vk.com/ru/method/messages.send
    // Детально не разбирался.
    // int code = json["response"].toInt(-1);
    // Может быть не только код, но и массивы объектов.
    // Потому просто вернем результат.

    return json;
}

static bool vkapi_returned_error(QJsonDocument& document, int& error_code, QString& error_msg) {
    bool has_error = false;

    qInfo() << document["error"]["error_code"].toInt(2);

    if (!document["error"]["error_code"].isUndefined()) {
        has_error = true;
        error_code = document["error"]["error_code"].toInt(-1);
    }

    if (!document["error"]["error_msg"].isUndefined()) {
        has_error = true;
        error_msg = document["error"]["error_msg"].toString("no description of error");
    }

    return has_error;
}

bool vk::get_user(const QString& vk_id, QString& first_name, QString& last_name, qint64& user_id, int& error_code, QString& error_msg) {
    QJsonDocument user_id_response = do_vkapi_request(
        "users.get",
        QVector<QString>{"user_ids"},
        QVector<QString>{vk_id}
        );

    error_code = 0; // 0 is no error for us, -1 is unknown error.
    error_msg.clear();

    if (vkapi_returned_error(user_id_response, error_code, error_msg)) {
        return false;
    }

    first_name = user_id_response["response"][0]["first_name"].toString("");
    last_name  = user_id_response["response"][0]["last_name"].toString("");

    user_id = user_id_response["response"][0]["id"].toInteger(0);
    if (user_id == 0) {
        // Неизвестная ошибка. ID не мог быть 0, мы его получили из-за того,
        // что значения не было или оно было не числом (или не поместилось, но
        // это вряд ли, если ответ корректный, все равно будем считать ошибкой).
        error_code = -1;
        error_msg = "unknown error";
        return false;
    }
}

bool vk::send_message(const QString& vk_id, const QString& content, qint64 uniqueness_id, int& error_code, QString& error_msg) {
    // https://dev.vk.com/ru/method/users.get
    // https://dev.vk.com/ru/method/messages.send


    QJsonDocument send_msg_response = do_vkapi_request(
        "messages.send",
        QVector<QString>{"user_id", "message", "random_id"},
        QVector<QString>{QString::number(user_id), content, QString::number(uniqueness_id)}
    );

    error_code = 0; // 0 is no error for us, -1 is unknown error.
    error_msg.clear();

    if (vkapi_returned_error(send_msg_response, error_code, error_msg)) {
        qInfo() << error_code << error_msg;
        return false;
    }

    return true;
}
