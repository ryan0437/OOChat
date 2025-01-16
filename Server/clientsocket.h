#ifndef CLIENTSCKET_H
#define CLIENTSCKET_H

#include "structures.h"

#include <QObject>
#include <QTcpSocket>
#include <QSet>
class DbManager;

//////////////////////////////////////////////
/// \brief The ClientSocket class
/// 单个客户端的消息交互处理
class ClientSocket : public QObject
{
    Q_OBJECT
public:
    explicit ClientSocket(QObject *parent = nullptr, QTcpSocket* client = nullptr);
    ~ClientSocket();

    QString g_Id;
    QTcpSocket* g_client;
    QSet<User> g_friendList;

    void processRequest();
    void sendJsonToClient(QJsonObject& jsonObj);                                // 将封装的JSON数据发送给客户端

signals:
    void signalDisconnected();                                                  // 与客户端断开连接
    void signalLoginSuccess();                                                  // 客户端验证账号密码登录成功
    void signalGotFriendList();                                                 // 获取到好友列表
    void signalDispatchMsg(const ChatMessage& message);                         // 转发消息给其他客户端
    void signalDispatchDeleted(QJsonObject& jsonObj);                           // 转发删除好友关系的消息
    void signalDispatchFriendEvent(QString& receiverId, QJsonObject& jsonObj);  // 转发好友事件
    void signalDispatchGroupEvent(QString& senderId, QString& groupId, QJsonObject& jsonObj);  //转发群组事件

private slots:
    void onReadyRead();

//处理客户端消息
private:
    // 客户端消息解析
    void parseLoginResponse(QJsonObject& jsonObj);                              // 登入请求：验证账号密码
    void parseLogoutUpdate(QJsonObject& jsonObj);                               // 客户端登出：修改数据库用户状态
    void parseGetFriendListResponse(QJsonObject& jsonObj);                      // 获取好友列表：展示在主界面
    void parseGetUserProfile(QJsonObject& jsonObj);                             // 获取用户个人资料
    void parseDispatchMsg(QJsonObject& jsonObj);                                // 转发用户要发送给其他用户的消息
    void parseGetGroupList(QJsonObject& jsonObj);                               // 获取用户的群组列表
    void parseRegister(QJsonObject& jsonObj);                                   // 用户注册
    void parseQueryUser(QJsonObject& jsonObj);                                  // 获取用户个人信息
    void parseQueryGroup(QJsonObject& jsonObj);                                 // 获取群组信息

    // 好友处理
    void getOfflineMessages();                                                  // 获取用户离线时接收到的消息
    void parseAddFriendRequest(QJsonObject& jsonObj);                           // 请求添加好友
    void notifyDeletedRelation(const QString& userId, const QString& sender);   // 告知用户被删除好友/提出群
    void parseDeleteFriendGroup(QJsonObject& jsonObj);                          // 删除好友/解散群组
    void parseUpdateFriendRemark(QJsonObject& jsonObj);                         // 添加好友备注
    void parseFriendRequestStatus(QJsonObject& jsonObj);                        // 接收者返回好友添加状态 同意/拒绝

    // 群组处理
    void parseRenameGroup(QJsonObject& jsonObj);                                // 群组重命名
    void parseJoinGroup(QJsonObject& jsonObj);                                  // 加入群组
    void parseQueryGroupMembers(QJsonObject& jsonObj);                          // 获取群组成员
    void parseCreateGroup(QJsonObject& jsonObj);                                // 创建群组

    DbManager *dbManager;

    QList<QString> containsImageType(const QString& content);                   // 获取聊天消息中的图片链接
};

#endif // CLIENTSCKET_H
