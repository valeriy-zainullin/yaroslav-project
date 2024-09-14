#ifndef VK_H
#define VK_H

#include <QString>

namespace vk {
    bool send_message(const QString& vk_id, const QString& content, int& error_code, QString& error_msg);
}

#endif // VK_H
