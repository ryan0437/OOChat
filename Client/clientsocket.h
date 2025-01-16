#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include "structures.h"

#include <QObject>

class QTcpSocket;
class QAbstractSocket;
class QFile;

///////////////////////////////////////////
/// \brief The ClientSocket class
/// 与服务器通信
class ClientSocket : public QObject
{
    Q_OBJECT
public:
    // 获取单例实例的方法
    static ClientSocket* instance();

    // 删除拷贝构造和赋值运算符，避免外部复制单例
    ClientSocket(const ClientSocket&) = delete;
    ClientSocket& operator=(const ClientSocket&) = delete;
    ~ClientSocket();

    QString g_Id;                       // 用户的Id
    User g_Profile;                     // 用户个人资料
    QHash<QString, User> g_friendList;  // 好友列表
    QHash<QString, Group> g_groupList;  // 群组列表

    bool getStatus();                   // 获得服务器连接状态
    // 向服务器请求数据
    void getFriendList();                                                               // 获取好友列表
    void getGroupList();                                                                // 获取群组列表
    void getGroupMember(const QString& groupId);                                        // 获取群组成员列表
    void sendLoginQuest(const QString& UserId, const QString& UserPwd);                 // 发送登录请求 密码校验
    void sendLogOut();                                                                  // 告知服务器登出（登入告知在服务器端验证密码成功时执行
    void getUserProfile(QString userId, std::function<void (const User&)> callback);    // 获取用户资料
    void getUserProfile(QString userId, int mode);                                      // 用户查询1：模糊查询， 0：精确查询
    void getGroupProfile(QString groupId, int mode);                                    // 群组查询1：模糊查询， 0：精确查询

    // 向服务器发送数据
    void sendMsg(const ChatMessage& message);                                           // 发送聊天消息
    void sendRegisterInfo(const Account& account);                                      // 发送注册请求
    void sendAddFriendRequest(const QString& userId, const QString& message);           // 发送添加好友请求
    void sendConfirmFriendRequest(const User& user, int status);                        // 发送接受/拒绝好友请求
    void deleteFriendGroup(const QString& id, const int role);                          // 删除好友/解散群组 role:0用户 1群组
    void addFriendRemark(const QString& userId, const QString& newName);                // 添加好友备注
    void createGroup(const Group& group);                                               // 创建群组
    void joinGroup(const QString& groupId);                                             // 加入群组
    void renameGroup(const QString& groupId, const QString& newName);                   // 重命名群组

signals:
    void signalConnected();                                                              // 连接到服务器
    void signalDisconnected();                                                           // 与服务器断开
    void signalLoginSuccessed();                                                         // 登录成功
    void signalLoginFailed();                                                            // 登录失败
    void signalGotFriendList();                                                          // 获取好友列表
    void signalUpdateStatus(QString& sender, int status);                                // 更新好友在线状态
    void signalGotProfile(const User& user);                                             // 获取用户资料
    void signalRecvMsg(ChatMessage& message);                                            // 接收聊天消息
    void signalGotGroupList();                                                           // 接收群组列表
    void signalGotUser(const QList<User> userList);                                      // 获取用户资料
    void signalGotGroup(const QList<Group> groupList);                                   // 获取群组资料
    void signalGotAddFriendReq(const User& user, const QString& msg, const QDate& date); // 接收到好友请求
    void signalDeleteChatWidget(const QString& userId);                                  // 删除好友后同时删除聊天窗口
    void signalDeletedRelation(const QString& senderId,int role);                        // 被删除好友/踢出群组
    void signalRenamedGroup(const QString& id, const QString& newName);                  // 群组被更新名字
    void signalUpdateFriendReq(const User& user, const int status);                      // 更新接收方返回的好友请求状态
    void signalAddedFriend(const QVariant& data);                                        // 被添加好友
    void signalGotGroupMembers(const QList<User>& memberList);                           // 获取到群组成员列表

private slots:
    void sltConnected();                                                                 // 连接上服务器
    void sltDisconnected();                                                              // 与服务器断开连接
    void sltReadyRead();                                                                 // 有可读消息

private:
    // 私有构造函数，确保外部不能直接创建实例
    explicit ClientSocket(QObject *parent = nullptr);

    // 解析服务器发来的消息
    void parseLoginQuest(QJsonObject& jsonObj);             // 解析登录请求结果
    void parsegetFriendList(QJsonObject& jsonObj);          // 解析获取的好友列表
    void parseUpdateStatus(QJsonObject& jsonObj);           // 解析好友变动的状态 上线/下线
    void parseUserProfile(QJsonObject& jsonObj);            // 解析获取用户个人资料
    void parseRecvMsg(QJsonObject& jsonObj);                // 解析接收消息
    void parseOfflineMessages(QJsonObject& jsonObj);        // 解析获取的离线时收到的消息
    void parseRecvFriendList(QJsonObject& jsonObj);         // 解析获取的好友列表
    void parseRecvUser(QJsonObject& jsonObj);               // 解析获取的用户资料
    void parseRecvGroup(QJsonObject& jsonObj);              // 解析获取的群组资料
    void parseRecvAddFriendReq(QJsonObject& jsonObj);       // 解析接收到的好友添加请求
    void parseRecvFriendReqStatus(QJsonObject& jsonObj);    // 解析自己发出的好友添加请求的响应者的回复
    void parseDeletedRelation(QJsonObject& jsonObj);        // 解析被删除好友消息
    void parseRenamedGroup(QJsonObject& jsonObj);           // 解析群组被重命名消息
    void parseGroupMembers(QJsonObject& jsonObj);           // 解析获取的群组成员列表

    // 用户个人与服务器交互事件
    void sendJsonToServer(QJsonObject& jsonObj);            // 发送封装好的JSON消息到服务器
    void getOfflineMessages();                              // 获取离线时接收到的消息

private:
    static ClientSocket* m_instance;    // 类单例
    QTcpSocket* m_client;               // tcpsocket
};

#endif // CLIENTSOCKET_H
