#ifndef SERVER_H
#define SERVER_H

#include "types.h"
#include "clientsocket.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QHash>
#include <QRunnable>
#include <QMutex>
class DbManager;

///////////////////////////////////////////////
/// \brief The Server class
/// 服务器类接收客户端连接、转发客户端消息
class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    ~Server();

signals:
    void signalUserLogin(const QString& userId);                                // 用户登录

private slots:
    void SltNewConnection();                                                    // 新连接
    void SltDisConnected();                                                     // 断开连接
    void sltDispatchMsg(const ChatMessage& message);                            // 转发消息
    void sltDispatchFriendEvent(QString& receiverId, QJsonObject& jsonObj);     // 转发用户事件
    void sltDispatchGroupEvent(QString& senderId, QString& groupId, QJsonObject& jsonObj);  // 转发群组事件

private:
    QTcpServer *m_tcpserver;
    DbManager *dbManager;

    void NotifyFriendStatus(const QString& UserId, const int& status);     // 告知好友上下线状态

    QHash<QString, ClientSocket*> m_clients;    //已连接的客户端列表
    QMutex m_mutex;
};

class ClientTask : public QRunnable
{
public:
    ClientTask(ClientSocket* clientSocket) : m_clientSocket(clientSocket){
        setAutoDelete(true);
    }

    void run() override{
        m_clientSocket->processRequest();
    }

private:
    ClientSocket* m_clientSocket;
};

#endif // SERVER_H
