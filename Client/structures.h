#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <QString>
#include <QHash>
#include <QJsonObject>
#include <QTime>
#include <QTimer>
#include <QMetaType>

// 用户在线or离线状态
enum Status {
    USER_ONLINE = 1,      // 在线
    USER_OFFLINE          // 离线
};

// 聊天消息类型
enum ChatTypes{
    USER = 1,           // 用户
    GROUP               // 群组
};

// 发送/接收到的好友请求状态
enum RequestStatus{
    Accepted = 1,           // 已接受：请求被当前用户接受
    Rejected,               // 已拒绝：请求被当前用户拒绝
    BeenRejected,           // 已被拒绝：发送方被对方拒绝
    BeenAccepted,           // 已被接收：发送方被对方接受
    WaitingForReceiver,     // 发送方等待接收者确认
    WaitingForSelf          // 接收方等待自己的确认操作
};

// 用户状态信息
struct User {
    QString id;               // 用户ID
    QString name;             // 用户名
    Status status;            // 在线状态
    QString avatarUUID;     // 头像路径

    // 默认构造函数
    User(QString userId = "0", const QString& userName = "", Status userStatus = USER_OFFLINE, const QString& iconPath = ":/imgs/head_icon7.jpg")
        : id(userId), name(userName), status(userStatus), avatarUUID(iconPath) {}

    // 重载==号 Set插入查找需要比较元素
    bool operator==(const User& other) const {
        return id == other.id && name == other.name && status == other.status && avatarUUID == other.avatarUUID;
    }

    // 将User封装为QJsonObject
        QJsonObject toJson() const {
            QJsonObject jsonObj;
            jsonObj["id"] = id;
            jsonObj["name"] = name;
            jsonObj["status"] = status;
            jsonObj["avatarUUID"] = avatarUUID;
            return jsonObj;
        }

    // 从QJsonObject解析User
    static User fromJson(const QJsonObject &jsonObj) {
        User user;
        user.id = jsonObj["id"].toString();
        user.name = jsonObj["name"].toString();
        user.status = static_cast<Status>(jsonObj["status"].toInt());
        user.avatarUUID = jsonObj["avatarUUID"].toString();
        return user;
    }
};
Q_DECLARE_METATYPE(User);

// 声明qHash函数
inline uint qHash(const User& user, uint seed = 0) {
    uint hash = qHash(user.id, seed);
    hash = qHash(user.name, hash);
    hash = qHash(static_cast<int>(user.status), hash);
    hash = qHash(user.avatarUUID, hash);
    return hash;
}

// 群组
struct Group{
    QString id;
    QString name;
    QString avatarUUID;

    Group(QString groupId = "", const QString& groupName = "", const QString& avatarUUID = ":/imgs/head_icon7.jpg")
        : id(groupId), name(groupName), avatarUUID(avatarUUID){}
};
Q_DECLARE_METATYPE(Group);

struct ChatMessage {
    enum MessageType {
        Text,
        File
    };

    MessageType type;        // 消息类型
    QString sender;          // 发送者
    QString receiver;        // 接收者ID
    ChatTypes chatType;      // 接收者类型
    QString text;            // 文本内容
    QString imagePath;       // 图片路径
    QString fileName;        // 文件名
    QString filePath;        // 文件路径
    qint64 fileSize;         // 文件大小（字节）
    QString fileUUID;        // 文件UUID
    bool isSent;             // 是否成功发送
    bool isDownloaded;       // 是否已下载
    bool isValid;            // 消息
    QDateTime time;          // 消息时间
    QString avatarUUID;      // 头像路径
};
Q_DECLARE_METATYPE(ChatMessage);

struct Account{
    QString account;        // 账号
    QString pwd;            // 密码
    QString name;           // 昵称
    QString avatarPath;     // 头像本地路径
    QString avatar;         // UUID
};

#endif // STRUCTURES_H
