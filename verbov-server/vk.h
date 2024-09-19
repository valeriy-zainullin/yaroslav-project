#ifndef VK_H
#define VK_H

#include <QString>

namespace vk {
    bool get_user(const QString& vk_id, QString& first_name, QString& last_name, qint64& user_id, int& error_code, QString& error_msg);
    bool send_message(const QString& vk_id, const QString& content, qint64 uniqueness_id, int& error_code, QString& error_msg);
}

#endif // VK_H
