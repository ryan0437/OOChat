#include "clientsocket.h"
#include "localdbmanager.h"
#include "types.h"

#include <QTcpSocket>
#include <QDebug>
#include <QAbstractSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QJsonArray>
#include <QFile>
#include <QDataStream>
#include <QFileInfo>
#include <QTimer>
#include <QUuid>

ClientSocket* ClientSocket::m_instance = nullptr;

ClientSocket::ClientSocket(QObject *parent) : QObject(parent), m_client(new QTcpSocket(this))
{
    // 尝试连接服务器
    m_client->connectToHost("127.0.0.1", 9999);

    //待添加：网络问题连接失败，重试功能。
    connect(m_client, &QTcpSocket::connected, this, &ClientSocket::sltConnected);
    connect(m_client, &QTcpSocket::disconnected, this, &ClientSocket::sltDisconnected);
    connect(m_client, &QTcpSocket::readyRead, this, &ClientSocket::sltReadyRead);
}

ClientSocket *ClientSocket::instance()
{
    if (!m_instance) m_instance = new ClientSocket;
    return m_instance;
}

ClientSocket::~ClientSocket()
{
    qDebug() << "clientsocket delete...";
    sendLogOut();                           // 告知服务器 客户端登出
    if (m_client->isOpen()) {
        m_client->close();
    }
    delete m_client;
}

bool ClientSocket::getStatus()
{
    return m_client->state() == QTcpSocket::ConnectedState;
}

void ClientSocket::sendLoginQuest(const QString &UserId, const QString &UserPwd)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_LOGIN_REQUEST;
    QJsonObject jsonData;
    jsonData["UserId"] = UserId;
    jsonData["UserPwd"] = UserPwd;
    jsonObj["account"] = jsonData;

    sendJsonToServer(jsonObj);
    qDebug() << "client sent login quest to server";
}

void ClientSocket::sendLogOut()
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_LOGOUT_RESPONSE;
    jsonObj["UserId"] = g_Id;

    sendJsonToServer(jsonObj);
    qDebug() << "clinet send logout event";
}

void ClientSocket::getUserProfile(QString userId, std::function<void (const User&)> callback)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_GET_USERPROFILE;
    jsonObj["sender"] = userId;

    sendJsonToServer(jsonObj);

    connect(this, &ClientSocket::signalGotProfile, this, [=](const User& user){
        callback(user);
        disconnect(this, &ClientSocket::signalGotProfile, this, nullptr);   // 只需要一次回调获取信息 调用一次后断开连接，销毁信号和槽连接（否则多次getprofile后每次都会链接出现混乱）
    });
}

void ClientSocket::getUserProfile(QString userId, int mode)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_QUERY_USER;
    jsonObj["userId"] = userId;
    jsonObj["mode"] = mode;

    sendJsonToServer(jsonObj);
    qDebug() << "client send user query: " << userId;
}

void ClientSocket::getGroupProfile(QString groupId, int mode)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_QUERY_GROUP;
    jsonObj["groupId"] = groupId;
    jsonObj["mode"] = mode;

    sendJsonToServer(jsonObj);
    qDebug() << "client send group query: " << groupId;
}

void ClientSocket::sltConnected()
{
    qDebug() << "connected successfully";
    emit signalConnected();
}

void ClientSocket::sltDisconnected()
{
    qDebug() << "Disconnected from the server";
    emit signalDisconnected();
}

void ClientSocket::sltReadyRead()
{
    // 解析定长包头4个字节
    static QByteArray buffer;
    buffer.append(m_client->readAll());

    while (true)
    {
        if (static_cast<size_t>(buffer.size()) < sizeof (int))
            return;

        size_t length = 0; // 正文长度
        memcpy(&length, buffer.constData(), sizeof (int)); // ！客户端和服务器需要采用相同的字节序，否则解析出的长度可能错误。

        if (static_cast<size_t>(buffer.size()) < sizeof (int) + length)
            return;

        QByteArray jsonData = buffer.mid(sizeof (int), length); // 提取正文为从四字节的报头开始长度为length
        buffer.remove(0, sizeof (int) + length);                // 本次的报文完整接收完毕，移除缓冲区内的本次报文数据

        qDebug() << "Received data:" << jsonData;

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

        if (parseError.error != QJsonParseError::NoError)
        {
            qDebug() << "Json Error:" << parseError.errorString();
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        int type = jsonObj["type"].toInt();

        switch (type) {
        case EVENT_LOGIN_RESPONSE:
            parseLoginQuest(jsonObj);
            break;
        case EVENT_RECV_FRIENDLIST:
            parsegetFriendList(jsonObj);
            break;
        case EVENT_RECV_FRIENDLOGIN:    // 同时处理好友上下线
        case EVENT_RECV_FRIENDLOGOUT:
            parseUpdateStatus(jsonObj);
            break;
        case EVENT_RECV_USERPROFILE:
            parseUserProfile(jsonObj);
            break;
        case EVENT_RECV_MESSAGE:
            parseRecvMsg(jsonObj);
            break;
        case EVENT_SEND_OFFLINEMESSAGES:
            parseOfflineMessages(jsonObj);
            break;
        case EVENT_RECV_GROUPLIST:
            parseRecvFriendList(jsonObj);
            break;
        case EVENT_RECV_USER:
            parseRecvUser(jsonObj);
            break;
        case EVENT_RECV_GROUP:
            parseRecvGroup(jsonObj);
            break;
        case EVENT_RECV_ADDFRIENDREQ:
            parseRecvAddFriendReq(jsonObj);
            break;
        case EVENT_DELETED_RELATION:
            parseDeletedRelation(jsonObj);
            break;
        case EVENT_RENAMED_GROUP:
            parseRenamedGroup(jsonObj);
            break;
        case EVENT_RECV_CONFIRM_FRIENDREQUEST:
            parseRecvFriendReqStatus(jsonObj);
            break;
        case EVENT_RECV_GROUPMEMBERS:
            parseGroupMembers(jsonObj);
            break;
        default:
            break;
        }
    }
}

void ClientSocket::parseLoginQuest(QJsonObject& jsonObj)
{
    bool isValid = jsonObj["status"].toBool();

    if (isValid)
        emit signalLoginSuccessed();
    else
        emit signalLoginFailed();
}

void ClientSocket::parsegetFriendList(QJsonObject& jsonObj)
{
    // 将好友列表从array转换成list，发送信号this->mainwindow->msglistview添加消息列表小窗口;
    QJsonArray jsonArray = jsonObj["data"].toArray();
    for(const QJsonValue& value : jsonArray)
    {
        QJsonObject userObj = value.toObject();
        QString id = userObj["id"].toString();
        QString name = userObj["name"].toString();
        int status = userObj["status"].toInt();
        QString avatar = userObj["avatar"].toString();

        User friendUser(id, name, static_cast<Status>(status), avatar);
        g_friendList[id] = friendUser;
    }
    emit signalGotFriendList();
    getOfflineMessages();   //  聊天窗口需要在获取用户信息之后才能初始化，随后获取离线收到的信息，并根据离线的信息将每个信息分配到对应的与发送者的聊天窗口
}

void ClientSocket::parseUpdateStatus(QJsonObject& jsonObj)
{
    QString sender = jsonObj["sender"].toString();

    int status = jsonObj["type"].toInt() == EVENT_RECV_FRIENDLOGIN ? 1 : 0;
    qDebug() << sender << "status:" << status;
    g_friendList[sender].status = static_cast<Status>(status);  // 更新好友状态

    emit signalUpdateStatus(sender, status);
}

void ClientSocket::parseUserProfile(QJsonObject& jsonObj)
{
    QJsonObject jsonData = jsonObj["data"].toObject();
    User user = User::fromJson(jsonData);
    emit signalGotProfile(user);
}

void ClientSocket::parseRecvMsg(QJsonObject& jsonObj)
{
    ChatMessage message;
    QString sender = jsonObj["sender"].toString();
    int chatType = jsonObj["chatType"].toInt();
    QString msg = jsonObj["msg"].toString();
    int messageType = jsonObj["messageType"].toInt();
    QDateTime time = QDateTime::fromString(jsonObj["time"].toString(), Qt::ISODate);

    message.type = static_cast<ChatMessage::MessageType>(messageType);
    message.sender = sender;
    message.chatType = static_cast<ChatTypes>(chatType);
    message.time = time;

    if (messageType == ChatMessage::Text)
        message.text = msg;
    else{
        QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toUtf8());
        if (jsonDoc.isObject())
        {
            QJsonObject fileObj = jsonDoc.object();
            message.fileName = fileObj["fileName"].toString();
            message.fileSize = fileObj["fileSize"].toVariant().toLongLong();
            message.fileUUID = fileObj["fileUUID"].toString();
            message.text = jsonDoc.toJson(QJsonDocument::Compact);
        }else{
            qDebug() << "parse failed...";
        }
    }

    if (chatType == ChatTypes::USER)
        message.receiver = jsonObj["receiver"].toString();
    else {
        message.receiver = jsonObj["recvGroup"].toString();
        message.avatarUUID = jsonObj["avatarUUID"].toString();
    }

    emit signalRecvMsg(message);
}

void ClientSocket::parseOfflineMessages(QJsonObject& jsonObj)
{
    QJsonArray jsonArray = jsonObj["data"].toArray();
    for(const QJsonValue& value : jsonArray)
    {
        ChatMessage message;
        QJsonObject msgObj = value.toObject();
        message.chatType = static_cast<ChatTypes>(msgObj["chatType"].toInt());
        message.sender = msgObj["sender"].toString();
        message.type = static_cast<ChatMessage::MessageType>(msgObj["messageType"].toInt());
        message.time = QDateTime::fromString(msgObj["time"].toString(), Qt::ISODate);
        QString msg = msgObj["message"].toString();

        if (message.type == ChatMessage::Text)
            message.text = msg;
        else if (message.type == ChatMessage::File){
            QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toUtf8());
            if (jsonDoc.isObject())
            {
                QJsonObject fileObj = jsonDoc.object();
                message.fileName = fileObj["fileName"].toString();
                message.fileSize = fileObj["fileSize"].toVariant().toLongLong();
                message.fileUUID = fileObj["fileUUID"].toString();
                message.text = jsonDoc.toJson(QJsonDocument::Compact);  // JSON文件信息在本地数据库内为文本方式储存
            }else{
                qDebug() << "parse failed...";
            }
        }

        if (message.chatType == ChatTypes::USER)
            message.receiver = msgObj["receiver"].toString();
        else {
            message.receiver = msgObj["recvGroup"].toString();;
            message.avatarUUID = msgObj["avatarUUID"].toString();
        }

        emit signalRecvMsg(message);
    }
}

void ClientSocket::parseRecvFriendList(QJsonObject& jsonObj)
{
    QJsonArray jsonArray = jsonObj["data"].toArray();
    for(const QJsonValue& value: jsonArray)
    {
        QJsonObject groupObj = value.toObject();
        QString groupId = groupObj["groupId"].toString();
        QString groupName = groupObj["groupName"].toString();
        QString avatarUUID = groupObj["avatarUUID"].toString();
        Group group(groupId, groupName, avatarUUID);
        g_groupList[groupId] = group;
    }
    emit signalGotGroupList();
}

void ClientSocket::parseRecvUser(QJsonObject& jsonObj)
{
    QJsonArray jsonArray = jsonObj["data"].toArray();
    QList<User> userList;
    qDebug() << "array size:" << jsonArray.size();
    for(const QJsonValue& value : jsonArray)
    {
        QJsonObject userObj = value.toObject();
        QString userId = userObj["userId"].toString();
        QString name = userObj["name"].toString();
        QString avatarUUID = userObj["avatarUUID"].toString();
        User user(userId, name, static_cast<Status>(0),avatarUUID);
        userList.append(user);
    }
    emit signalGotUser(userList);
}

void ClientSocket::parseRecvGroup(QJsonObject &jsonObj)
{
    QJsonArray jsonArray = jsonObj["data"].toArray();
    QList<Group> groupList;
    qDebug() << "array size:" << jsonArray.size();
    for(const QJsonValue& value : jsonArray)
    {
        QJsonObject groupObj = value.toObject();
        QString groupId = groupObj["groupId"].toString();
        QString groupName = groupObj["groupName"].toString();
        QString avatarUUID = groupObj["avatarUUID"].toString();
        Group group(groupId, groupName, avatarUUID);
        groupList.append(group);
    }
    emit signalGotGroup(groupList);
}

void ClientSocket::parseRecvAddFriendReq(QJsonObject& jsonObj) // 接收到好友添加请求
{
    User user;
    user.id = jsonObj["sender"].toString();
    user.name = jsonObj["name"].toString();
    user.avatarUUID = jsonObj["avatarUUID"].toString();
    QString message = jsonObj["message"].toString();
    QString dateStr = jsonObj["date"].toString();
    QDate date = QDate::fromString(dateStr, "yyyy-MM-dd");

    emit signalGotAddFriendReq(user, message, date);
}

void ClientSocket::parseRecvFriendReqStatus(QJsonObject &jsonObj) // 被添加者返回的信息 拒绝/同意
{
    QString senderId = jsonObj["sender"].toString();
    QString senderName = jsonObj["name"].toString();
    int status = jsonObj["status"].toInt();
    QString avatarUUID = jsonObj["avatarUUID"].toString();
    User user(senderId, senderName, static_cast<Status>(status), avatarUUID);

    if (!g_friendList.contains(senderId))
    {
        QVariant userVariant = QVariant::fromValue(user);
        emit signalAddedFriend(userVariant);
    }

    LocalDbManager::instance().updateRequestStatus(g_Id, user.id, status);
}

void ClientSocket::parseDeletedRelation(QJsonObject& jsonObj)
{
    QString sender = jsonObj["sender"].toString();
    g_friendList.remove(sender);
    emit signalDeletedRelation(sender, 0);
}

void ClientSocket::parseRenamedGroup(QJsonObject& jsonObj)
{
    QString groupId = jsonObj["groupId"].toString();
    QString newName = jsonObj["newName"].toString();
    emit signalRenamedGroup(groupId, newName);
}

void ClientSocket::parseGroupMembers(QJsonObject &jsonObj)
{
    QList<User> memberList;
    QJsonArray jsonArray = jsonObj["data"].toArray();
    for(const QJsonValue &value : jsonArray) {
        QJsonObject userObj = value.toObject();
        QString id = userObj["id"].toString();
        QString name = userObj["name"].toString();
        int status = userObj["status"].toInt();
        QString avatarUUID = userObj["avatarUUID"].toString();
        User user(id, name, static_cast<Status>(status), avatarUUID);
        memberList.append(user);
    }
    emit signalGotGroupMembers(memberList);
}

void ClientSocket::sendJsonToServer(QJsonObject &jsonObj)
{
    // 给每条消息创建uuid标识 目前没什么用...
    QUuid uuid = QUuid::createUuid();
    QString uuidStr = uuid.toString();
    jsonObj["uuid"] = uuidStr;

    //转化为json文档
    QJsonDocument jsonDoc(jsonObj);
    QByteArray dataByte = jsonDoc.toJson(QJsonDocument::Compact);

    int length = dataByte.size();
    QByteArray header;
    header.append(reinterpret_cast<const char*>(&length), sizeof (int));

    // 发送JSON数据给服务器
    if (m_client->state() == QAbstractSocket::ConnectedState)
    {
        m_client->write(header + dataByte);
        m_client->flush(); //确保缓冲区中的数据立即发送到服务器，强制数据发送，确保在调用之后缓冲区内容已经被尽可能地发送到目标地址
    }
}

void ClientSocket::sendMsg(const ChatMessage& message)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_SEND_MESSAGE;
    jsonObj["sender"] = message.sender;
    jsonObj["receiver"] = message.receiver;
    jsonObj["chatType"] = message.chatType;
    jsonObj["msg"] = message.text;
    jsonObj["messageType"] = message.type;
    jsonObj["time"] = message.time.toString(Qt::ISODate);

    sendJsonToServer(jsonObj);
    qDebug() << "send msg to " << message.receiver << "  length:" << message.text.length();
}

void ClientSocket::sendRegisterInfo(const Account &account)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_REQUEST_REGISTER;
    jsonObj["account"] = account.account;
    jsonObj["pwd"] = account.pwd;
    jsonObj["name"]= account.name;
    jsonObj["avatarUUID"] = account.avatar;

    sendJsonToServer(jsonObj);
    qDebug() << "sent register request....";
}

void ClientSocket::sendAddFriendRequest(const QString &userId, const QString& message)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_REQUEST_ADD_FRIEND;
    jsonObj["sender"] = g_Id;
    jsonObj["receiver"] = userId;
    jsonObj["message"] = message;
    jsonObj["date"] = QDate::currentDate().toString("yyyy-MM-dd");

    sendJsonToServer(jsonObj);
    qDebug() << "send add friend request to " << userId;
}

void ClientSocket::sendConfirmFriendRequest(const User& user, int status)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_CONFIRM_FRIENDREQUEST;
    jsonObj["sender"] = g_Id;
    jsonObj["receiver"] = user.id;
    jsonObj["status"] = status;

    sendJsonToServer(jsonObj);
    qDebug() << "send friend request confirmation to " << user.id << " status:" << status;
    if (status == RequestStatus::Accepted)
        g_friendList[user.id] = user;
}

void ClientSocket::getFriendList()
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_GET_FRIENDLIST;
    jsonObj["UserId"] = g_Id;

    sendJsonToServer(jsonObj);
    qDebug() << "send get friend list request";
}

void ClientSocket::getGroupList()
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_GET_GROUPLIST;
    jsonObj["userId"] = g_Id;

    sendJsonToServer(jsonObj);
}

void ClientSocket::getGroupMember(const QString& groupId)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_GET_GROUPMEMBERS;
    jsonObj["userId"] = g_Id;
    jsonObj["groupId"] = groupId;

    sendJsonToServer(jsonObj);
}

void ClientSocket::getOfflineMessages()
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_RECV_OFFLINEMESSAGES;
    jsonObj["sender"] = g_Id;

    sendJsonToServer(jsonObj);
}

void ClientSocket::deleteFriendGroup(const QString &id, const int role)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_DELETE_FRIENDGROUP;
    jsonObj["role"] = role;
    jsonObj["senderId"] = g_Id;
    jsonObj["friendId"] = id;

    sendJsonToServer(jsonObj);

    if (role == 0) // user
        g_friendList.remove(id);
    else    // group
        g_groupList.remove(id);

    emit signalDeleteChatWidget(id);
}

void ClientSocket::addFriendRemark(const QString &userId, const QString &newName)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_ADD_FRIENDREMARK;
    jsonObj["sender"] = g_Id;
    jsonObj["receiver"] = userId;
    jsonObj["newName"] = newName;

    sendJsonToServer(jsonObj);
}

void ClientSocket::createGroup(const Group &group)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_CREATE_GROUP;
    jsonObj["sender"] = g_Id;
    jsonObj["groupId"] = group.id;
    jsonObj["groupName"] = group.name;
    jsonObj["avatarUUID"] = group.avatarUUID;

    sendJsonToServer(jsonObj);
}

void ClientSocket::joinGroup(const QString &groupId)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_JOIN_GROUP;
    jsonObj["sender"] = g_Id;
    jsonObj["groupId"] = groupId;

    sendJsonToServer(jsonObj);
}

void ClientSocket::renameGroup(const QString &groupId, const QString &newName)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_RENAME_GROUP;
    jsonObj["groupId"] = groupId;
    jsonObj["newName"] = newName;

    sendJsonToServer(jsonObj);
}
