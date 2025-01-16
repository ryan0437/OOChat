#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QList>
#include <QMutex>
#include <QQueue>
#include <QWaitCondition>

class ChatMessage;
class Account;
class Group;
class User;

////////////////////////////////////////////////
/// \brief The DbManager class
/// 服务器数据库管理，增删查改
class DbManager
{
public:
    DbManager();
    ~DbManager();

    QSqlDatabase createConnection(const QString &connectionName);                               // 为每个线程创建数据库对象
    bool checkUserCredential(const QString& UserId, const QString& UserPwd);                    // 检查用户账号密码是否正确
    QList<User> getFriendList(const QString& UserId);                                           // 获取用户好友列表
    void updateUserStatus(const QString& UserId, int status);                                   // 更新用户在线状态
    int getUserStatus(const QString& UserId);                                                   // 获取用户的在线状态
    User getUserProfile(const QString& UserId);                                                 // 获取用户个人资料
    void saveOfflineMessages(const QJsonObject& jsonObj);                                       // 储存用户离线时收到的信息
    QJsonArray getOfflineMessages(const QString& userId);                                       // 获取用户离线时收到的信息
    QJsonArray getGroupList(const QString& userId);                                             // 获取用户群组列表
    QJsonArray getGroupMembers(const QString& groupId);                                         // 获取群组成员列表
    QList<QString> getGroupALlUsers(const QString& groupId);                                    // 获取群组中所有用户的ID
    void saveFile(const ChatMessage& message);                                                  // 保存用户上传的文件
    ChatMessage getFileInfo(const QString& fileUUID);                                           // 获取用户请求下载的文件信息
    void saveRegisterInfo(const Account& account);                                              // 保存注册用户信息
    QJsonArray getUserInfo(const QString& userId, int mode);                                    // 查询用户信息mode 1：模糊查询， 0：精确查询
    QJsonArray getGroupInfo(const QString& groupId, int mode);                                  // 查询群组信息mode 1：模糊查询， 0：精确查询
    void addFriendRelation(const QString& userA, const QString& userB);                         // 添加好友关系
    void addGroupRelation(const QString& groupId, const QString& userId);                       // 添加用户加入群组关系
    void deleteFriendGroup(const QString& idA, const QString& idB, int role);                   // 删除好友/解散群组 role:0好友 1群组
    void deleteOfflineMsg(const QString& senderId, const QString& receiverId, int role);        // 删除已经发送/需要舍弃 的离线消息
    void createGroup(const Group& group);                                                       // 创建群组 保存群组信息
    void quitGroup(const QString& groupId, const QString& userId);                              // 退出群组
    void renameGroup(const QString& groupId, const QString& newName);                           // 更新群组名称
    void saveOfflineEvent(const QString& userId, const QJsonObject& jsonObj);                   // 保存离线时接收到的活动事件
    QList<QJsonObject> getOfflineEvent(const QString& userId);                                  // 获取离线时接收到的活动事件

private:
    QSqlDatabase getConnection();                                                               // 获取连接池中的空闲链接
    void releaseConnection(QSqlDatabase db);                                                    // 释放连接池的链接
};

/////////////////////////////////////////////////////
/// \brief The DatabasePool class 数据库连接池
///
class DatabasePool {
public:
    static DatabasePool& instance();                    // 返回单例的数据库连接池实例
    ~DatabasePool();                                    // 释放数据库连接池资源

    QSqlDatabase getConnection();                       // 获取数据库连接
    void releaseConnection(QSqlDatabase connection);    // 释放数据库连接

    void initDatabase();                                // 初始化数据库，创建需要的表

private:
    DatabasePool();                                     // 初始化数据库连接池
    void createConnection(int count);                   // 创建指定数量的数据库连接并加入连接池

    QQueue<QSqlDatabase> connectionQueue;               // 数据库连接队列
    QMutex mutex;                                       // 互斥锁同步对连接池的访问
    QWaitCondition waitCondition;                       // 连接池无空闲资源时等待

    int maxConnections;                                 // 最大数据库连接数
    QString databaseName;                               // 数据库名称
    QString hostName;                                   // 数据库主机名
    int port;                                           // 数据库端口号
    QString userName;                                   // 数据库用户名
    QString password;                                   // 数据库密码
};

#endif // DATABASE_H
