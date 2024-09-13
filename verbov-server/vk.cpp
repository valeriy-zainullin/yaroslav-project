#include "vk.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

// TODO: ____ !!!!обязательно поменять на токен Ярика!!!!! ____
// И пусть он создаст сообщество.

// Group token. Should have messages permission.
static const QString vk_group_token = "vk1.a.aUOQhDoQOBcr_FvQp77g9HhCi3PCm9LygaY1b275QKtVwXUXWAI5lMw6-YijCsj_VXwfPwmnYZKy8drof1jOf7AQM5K-mvfpOv-KG26WkCXvkMUA80asrezEtozqFi_FZUZ_KddK-NrKtJNXIlkBffgBcARP_sjn8a2L2-Gug6rqeeUeuk0oQ94f25ecztXerXp4-QxYZ5n1gqT6irtFsQ";

static int do_vkapi_request(QString method, QVector<QString> args, QVector<QString> values) {
    // https://dev.vk.com/ru/api/api-requests
    // https://stackoverflow.com/questions/46943134/how-do-i-write-a-qt-http-get-request

    // Не выделяем на стеке, т.к. объект может быть большой.
    // Выделяем на куче, даем владеть умному указателю, который сам освободит.
    QScopedPointer<QNetworkAccessManager> netmanager = new QNetworkAccessManager();

    // Этот выделяют на стеке. Видимо, он не такой уж и большой.
    // Заходим внутрь и видим, что внутри много методов и лишь один
    // указатель на какой-то другой объект. Уж 8 байт можно на стеке :)
    QNetworkRequest request;

    // https://forum.qt.io/topic/137547/sending-parameters-by-get-method-to-rest-api/6
    QUrl url("https://api.vk.ru/method/" + method);
    QUrlQuery query;
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

    QByteArray result = reply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(result);

    // Похоже, что у vk api все коды ошибок положительные. Судя по
    // https://dev.vk.com/ru/method/messages.send
    // Детально не разбирался.
    int code = json["response"].toInt(-1);

    return code;
}

int vk::send_message(QString vk_id, QString content) {
    // https://dev.vk.com/ru/method/users.get
    // https://dev.vk.com/ru/method/messages.send

}
