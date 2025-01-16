#include "tcpserver.h"
#include "dbmanager.h"

#include <QHostAddress>
#include <QDebug>
#include <QTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThreadPool>

Server::Server(QObject *parent) : QObject(parent)
{
    m_tcpserver = new QTcpServer(this);
    dbManager = new DbManager;
    bool ok = m_tcpserver->listen(QHostAddress::Any, 9999);
    if (!ok) return;
    qDebug() << "main server is listening port 9999";

    DatabasePool& pool = DatabasePool::instance();
    QSqlDatabase db = pool.getConnection();
    if (db.isOpen())
        qDebug() << "连接池创建成功";
    else
        qDebug() << "连接池创建失败";
    pool.releaseConnection(db);

    connect(m_tcpserver, &QTcpServer::newConnection, this, &Server::SltNewConnection);
}

Server::~Server()
{}

void Server::NotifyFriendStatus(const QString &UserId, const int &status)
{
    if (!m_clients.contains(UserId) || !m_clients[UserId]) { //如果 UserId 不存在于 m_clients 中，则 m_clients[UserId] 会创建一个默认的键值对nullptr，防止访问空对象崩溃
        qDebug() << "Invalid client or user ID: " << UserId;
        return;
    }

    const ClientSocket* client = m_clients[UserId];
    for (const User& user : client->g_friendList)
    {
        if (!m_clients.contains(user.id)) continue; // 检查好友是否在线

        ClientSocket* online_friend = m_clients[user.id];
        if (!online_friend || online_friend->g_client->state() != QAbstractSocket::ConnectedState) continue;

        QJsonObject jsonObj;
        jsonObj["type"] = status == 1 ? EVENT_RECV_FRIENDLOGIN : EVENT_RECV_FRIENDLOGOUT;
        jsonObj["sender"] = UserId;
        jsonObj["receiver"] = user.id;

        online_friend->sendJsonToClient(jsonObj);
        qDebug() << "Notified " << UserId << "'s status " << status << " to online friend " << user.id;
    }
}


void Server::SltDisConnected()
{

}

void Server::sltDispatchMsg(const ChatMessage &message)
{
    // 区分接收者（用户or群组）
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_RECV_MESSAGE;
    jsonObj["sender"] = message.sender;
    jsonObj["msg"] = message.text;
    jsonObj["chatType"] = message.chatType;
    jsonObj["messageType"] = message.type;
    jsonObj["time"] = message.time.toString(Qt::ISODate);
    if (message.chatType == ChatTypes::USER)
    {
        jsonObj["receiver"] = message.receiver;
        if (m_clients.contains(message.receiver))   // 接收者在线：直接发送消息
        {
            ClientSocket* recvSocket = m_clients[message.receiver];
            recvSocket->sendJsonToClient(jsonObj);
        }else{
            dbManager->saveOfflineMessages(jsonObj);
        }
    }
    else
    {
        jsonObj["recvGroup"] = message.receiver;
        jsonObj["avatarUUID"] = message.avatarUUID;
        qDebug() << message.receiver;
        // 若接收者为群组，首先获得群内所有成员，再判断当前群成员是否在线（在线直接发送，不在线则存入离线消息表）
        QList<QString> groupUserName = dbManager->getGroupALlUsers(message.receiver);

        for(const QString& name : groupUserName)
        {
            if (name == message.sender) continue;  // 排除发送者
            qDebug() << name;

            jsonObj["receiver"] = name;
            if (m_clients.contains(name))
            {
                ClientSocket *recvSocket = m_clients[name];
                recvSocket->sendJsonToClient(jsonObj);
                qDebug() << "发送到群组：" << jsonObj;
            }else{
                QJsonObject groupJsonObj;
                groupJsonObj["senderAvatar"] = message.avatarUUID;
                groupJsonObj["message"] = message.text;
                QJsonDocument groupJsonDoc(groupJsonObj);
                QString groupMessageStr = groupJsonDoc.toJson(QJsonDocument::Compact);
                QJsonObject offlineJsonObj = jsonObj;
                offlineJsonObj["msg"] = groupMessageStr;
                dbManager->saveOfflineMessages(offlineJsonObj);
            }
        }
    }
}

void Server::sltDispatchFriendEvent(QString &receiverId, QJsonObject &jsonObj)
{
    if (m_clients.contains(receiverId)){
        ClientSocket* online_receiver = m_clients[receiverId];
        if (jsonObj["type"].toInt() == EVENT_RECV_CONFIRM_FRIENDREQUEST)    // 给接收者添加新好友信息到列表
            online_receiver->g_friendList.insert(dbManager->getUserProfile(jsonObj["sender"].toString()));

        online_receiver->sendJsonToClient(jsonObj);
    } else {
        qDebug() << "receiver:" << receiverId << "is offline!";
        dbManager->saveOfflineEvent(receiverId, jsonObj);
    }
}

void Server::sltDispatchGroupEvent(QString& senderId, QString& groupId, QJsonObject& jsonObj)
{
    QList<QString> groupUserName = dbManager->getGroupALlUsers(groupId);
    for(const QString& name : groupUserName)
    {
        if (name == senderId) continue;  // 排除发送者
        if (m_clients.contains(name))
        {
            ClientSocket *recvSocket = m_clients[name];
            recvSocket->sendJsonToClient(jsonObj);
            qDebug() << "发送到群组：" << jsonObj;
        }else{
            dbManager->saveOfflineMessages(jsonObj);
        }
    }
}

void Server::SltNewConnection()
{
    QTcpSocket* new_client = m_tcpserver->nextPendingConnection();
    ClientSocket* client = new ClientSocket(this, new_client);
    qDebug() << "new connection";

    ClientTask* task = new ClientTask(client);
    QThreadPool::globalInstance()->start(task);

    connect(client, &ClientSocket::signalLoginSuccess, this, [=](){
        QMutexLocker locker(&m_mutex);
        m_clients[client->g_Id] = client;
        qDebug() << "User " << client->g_Id << " login successfully at " << QTime::currentTime().toString();
        emit signalUserLogin(client->g_Id);
    });

    connect(client, &ClientSocket::signalGotFriendList, this, [=](){
        NotifyFriendStatus(client->g_Id, 1);
    });

    connect(client, &ClientSocket::signalDisconnected, this, [=](){
        QMutexLocker locker(&m_mutex);
        NotifyFriendStatus(client->g_Id, 0);
        m_clients.remove(client->g_Id);     // 用户离线，服务器端HASH表删除该用户
        client->deleteLater();
        qDebug() << "User " << client->g_Id << " logout at " << QTime::currentTime().toString();
    });

    connect(client, &ClientSocket::signalDispatchMsg, this, &Server::sltDispatchMsg);
    connect(client, &ClientSocket::signalDispatchFriendEvent, this, &Server::sltDispatchFriendEvent);
    connect(client, &ClientSocket::signalDispatchGroupEvent, this, &Server::sltDispatchGroupEvent);
}
