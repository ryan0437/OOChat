#ifndef LOCALDBMANAGER_H
#define LOCALDBMANAGER_H

#include <QObject>
#include <QSqlDatabase>

class User;
class ChatMessage;

////////////////////////////////////////////////////////
/// \brief The LocalDbManager class
/// 本地数据库
/// 储存历史聊天信息、历史好友添加请求
class LocalDbManager : public QObject
{
    Q_OBJECT
public:
    explicit LocalDbManager(QObject *parent = nullptr);
    ~LocalDbManager();

    static LocalDbManager &instance();

    void saveMessage(const ChatMessage& message);                                               // 保存聊天记录到本地
    QList<ChatMessage> getHistoryMessage(const QString& userOrGroupId, int chatType);           // 获取历史聊天记录
    ChatMessage getLastestMsg(const QString& userId);                                           // 获取最近的一条聊天信息

    void saveRequest(const User& user, const QString& msg, const QDate& date, int status);      // 保存好友请求记录
    void updateRequestStatus(const QString& senderId, const QString& receiverId, int status);   // 更新好友请求状态
    QList<QVariantMap> getHistoryRequest();                                                     // 获取历史请求


private:
    void initDatabase();                                                                        // 初始化数据库创建表

private:
    LocalDbManager(const LocalDbManager&) = delete;
    LocalDbManager& operator=(const LocalDbManager&) = delete;

    QSqlDatabase db;

    QString m_selfId;
};

#endif // LOCALDBMANAGER_H
