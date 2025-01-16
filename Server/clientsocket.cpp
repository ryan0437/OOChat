#include "clientsocket.h"
#include "types.h"
#include "dbmanager.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>

ClientSocket::ClientSocket(QObject *parent, QTcpSocket *client) :
    QObject(parent),
    g_client(client),
    dbManager(new DbManager)
{}

ClientSocket::~ClientSocket()
{
}

void ClientSocket::processRequest()
{
    connect(g_client, &QTcpSocket::disconnected, this, [=](){emit signalDisconnected();});
    connect(g_client, &QTcpSocket::readyRead, this, &ClientSocket::onReadyRead);
}

void ClientSocket::parseLoginResponse(QJsonObject& jsonObj)
{
    //数据库查询
    QJsonObject jsonData = jsonObj["account"].toObject();
    QString UserId = jsonData["UserId"].toString();
    QString UserPwd = jsonData["UserPwd"].toString();
    bool isValid = dbManager->checkUserCredential(UserId, UserPwd);

    QJsonObject jsonObjRes;
    jsonObjRes["type"] = EVENT_LOGIN_RESPONSE;
    QJsonObject jsonDataRes;
    jsonObjRes["status"] = isValid;

    sendJsonToClient(jsonObjRes);
    qDebug() << "send response to client....";
    if (isValid) g_Id = UserId;
    emit signalLoginSuccess();
}

void ClientSocket::parseLogoutUpdate(QJsonObject &jsonObj)
{
    QString UserId = jsonObj["UserId"].toString();
    dbManager->updateUserStatus(UserId, EVENT_LOGOUT_RESPONSE);
}

void ClientSocket::parseGetFriendListResponse(QJsonObject& jsonObj)
{
    QString UserId = jsonObj["UserId"].toString();
    QList<User> friendList = dbManager->getFriendList(UserId);

    QJsonObject jsonObjRes;
    jsonObjRes["type"] = EVENT_RECV_FRIENDLIST;
    QJsonArray list;

    for(const User& user : friendList)
    {
        QJsonObject userObj;
        userObj["id"] = user.id;
        userObj["name"] = user.name;
        userObj["status"] = user.status;
        userObj["avatar"] = user.avatarUUID;
        list.append(userObj);
        //qDebug() << user.id << " " << user.name << " " << user.status;
        g_friendList.insert(user);
    }
    jsonObjRes["data"] = list;

    sendJsonToClient(jsonObjRes);
    qDebug() << "response to get friend list";
    emit signalGotFriendList();
}

void ClientSocket::parseGetUserProfile(QJsonObject &jsonObj)
{
    QString sender = jsonObj["sender"].toString();
    User user = dbManager->getUserProfile(sender);

    QJsonObject jsonObjRes, jsonData;
    jsonObjRes["type"] = EVENT_RECV_USERPROFILE;
    jsonData = user.toJson();   // 将User结构体转化为JSON格式
    jsonObjRes["data"] = jsonData;

    sendJsonToClient(jsonObjRes);
}

void ClientSocket::parseDispatchMsg(QJsonObject &jsonObj)
{
    QString sender = jsonObj["sender"].toString();
    QString receiver = jsonObj["receiver"].toString();
    int chatType = jsonObj["chatType"].toInt();
    int messageType = jsonObj["messageType"].toInt();
    QDateTime time = QDateTime::fromString(jsonObj["time"].toString(), Qt::ISODate);

    ChatMessage message;
    message.type = ChatMessage::Text;
    message.sender = sender;
    message.receiver = receiver;
    message.chatType = static_cast<ChatTypes>(chatType);
    message.text = jsonObj["msg"].toString();
    message.type = static_cast<ChatMessage::MessageType>(messageType);
    message.time = time;
    if (message.chatType == ChatTypes::GROUP) {
        User senderProfile = dbManager->getUserProfile(sender);
        message.avatarUUID = senderProfile.avatarUUID;
    }

    emit signalDispatchMsg(message);
}

void ClientSocket::parseGetGroupList(QJsonObject &jsonObj)
{
    QString userId = jsonObj["userId"].toString();
    QJsonObject jsonObjRes;
    jsonObjRes["type"] = EVENT_RECV_GROUPLIST;
    QJsonArray jsonArray = dbManager->getGroupList(userId);
    jsonObjRes["data"] = jsonArray;

    sendJsonToClient(jsonObjRes);
}

void ClientSocket::parseRegister(QJsonObject &jsonObj)
{
    Account accountInfo;
    accountInfo.account = jsonObj["account"].toString();
    accountInfo.pwd = jsonObj["pwd"].toString();
    accountInfo.name = jsonObj["name"].toString();
    accountInfo.avatar = jsonObj["avatarUUID"].toString();
    dbManager->saveRegisterInfo(accountInfo);
}

void ClientSocket::parseQueryUser(QJsonObject &jsonObj)
{
    QString userId = jsonObj["userId"].toString();
    int mode = jsonObj["mode"].toInt(); // 1：模糊查询， 0：精确查询

    QJsonObject jsonObjRes;
    jsonObjRes["type"] = EVENT_RECV_USER;
    QJsonArray jsonArray = dbManager->getUserInfo(userId, mode);
    jsonObjRes["data"] = jsonArray;

    sendJsonToClient(jsonObjRes);
}

void ClientSocket::parseQueryGroup(QJsonObject &jsonObj)
{
    QString groupId = jsonObj["groupId"].toString();
    int mode = jsonObj["mode"].toInt();

    QJsonObject jsonObjRes;
    jsonObjRes["type"] = EVENT_RECV_GROUP;
    QJsonArray jsonArray = dbManager->getGroupInfo(groupId, mode);
    jsonObjRes["data"] = jsonArray;

    sendJsonToClient(jsonObjRes);
}

void ClientSocket::parseDeleteFriendGroup(QJsonObject &jsonObj)
{
    int role = jsonObj["role"].toInt();
    QString senderId = jsonObj["senderId"].toString();
    QString friendId = jsonObj["friendId"].toString();


    if (role == 0)
    {
        dbManager->deleteFriendGroup(senderId, friendId, role);
        notifyDeletedRelation(friendId, senderId);
    }else
        dbManager->quitGroup(friendId, senderId);
}

void ClientSocket::parseUpdateFriendRemark(QJsonObject &jsonObj)
{
    QString sender = jsonObj["sender"].toString();
    QString receiver = jsonObj["receiver"].toString();
    QString newName = jsonObj["newName"].toString();

    // 待实现添加好友名字备注
    //dbManager->updateFriendRemark(sender, receiver, newName);
}

void ClientSocket::parseRenameGroup(QJsonObject &jsonObj)
{
    QString groupId = jsonObj["groupId"].toString();
    QString newName = jsonObj["newName"].toString();

    dbManager->renameGroup(groupId, newName);

    jsonObj["type"] = EVENT_RENAMED_GROUP;
    emit signalDispatchGroupEvent(g_Id, groupId, jsonObj);
}

void ClientSocket::parseJoinGroup(QJsonObject &jsonObj)
{
    QString sender = jsonObj["sender"].toString();
    QString groupId = jsonObj["groupId"].toString();

    dbManager->addGroupRelation(groupId, sender);
}

void ClientSocket::parseQueryGroupMembers(QJsonObject &jsonObj)
{
    QString groupId = jsonObj["groupId"].toString();

    QJsonObject jsonObjRes;
    jsonObjRes["type"] = EVENT_RECV_GROUPMEMBERS;
    jsonObjRes["data"] = dbManager->getGroupMembers(groupId);

    sendJsonToClient(jsonObjRes);
}

void ClientSocket::parseCreateGroup(QJsonObject &jsonObj)
{
    QString groupId = jsonObj["groupId"].toString();
    QString groupName = jsonObj["groupName"].toString();
    QString avatarUUID = jsonObj["avatarUUID"].toString();
    Group group(groupId, groupName, avatarUUID);
    dbManager->createGroup(group);
}

QList<QString> ClientSocket::containsImageType(const QString &content)
{
    QList<QString> imageList;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(content.toUtf8());
    QJsonArray jsonArray = jsonDoc.array();
    for (const QJsonValue &value : jsonArray) {
        if (value.isObject()) {
            QJsonObject jsonObj = value.toObject();
            if (jsonObj.contains("type") && jsonObj["type"].toString() == "image") {
                QString imagePath = jsonObj["content"].toString();
                imageList.append(imagePath);
            }
        }
    }
    return imageList;
}

void ClientSocket::parseFriendRequestStatus(QJsonObject &jsonObj)
{
    QString sender = jsonObj["sender"].toString();
    QString receiver = jsonObj["receiver"].toString();
    int status = jsonObj["status"].toInt();
    User user = dbManager->getUserProfile(sender);
    jsonObj["type"] = EVENT_RECV_CONFIRM_FRIENDREQUEST;
    jsonObj["name"] = user.name;
    jsonObj["status"] = user.status;
    jsonObj["avatarUUID"] = user.avatarUUID;

    if (status == RequestStatus::Accepted)
    {
        g_friendList.insert(dbManager->getUserProfile(receiver));
        dbManager->addFriendRelation(sender, receiver);
    }
    emit signalDispatchFriendEvent(receiver, jsonObj);
}

void ClientSocket::getOfflineMessages()
{
    QJsonArray jsonArray = dbManager->getOfflineMessages(g_Id);
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_SEND_OFFLINEMESSAGES;
    jsonObj["data"] = jsonArray;

    sendJsonToClient(jsonObj);

    QList<QJsonObject> offlineEventList = dbManager->getOfflineEvent(g_Id);
    for(QJsonObject jsonObj : offlineEventList)
        sendJsonToClient(jsonObj);
}

void ClientSocket::parseAddFriendRequest(QJsonObject &jsonObj)
{
    jsonObj["type"] = EVENT_RECV_ADDFRIENDREQ;
    QString sender = jsonObj["sender"].toString();
    QString receiver = jsonObj["receiver"].toString();
    User senderProfile = dbManager->getUserProfile(sender);
    jsonObj["name"] = senderProfile.name;
    jsonObj["avatarUUID"] = senderProfile.avatarUUID;

    emit signalDispatchFriendEvent(receiver, jsonObj);
}

void ClientSocket::notifyDeletedRelation(const QString &userId, const QString &sender)
{
    QJsonObject jsonObj;
    jsonObj["type"] = EVENT_DELETED_RELATION;
    jsonObj["sender"] = sender;
    jsonObj["receiver"] = userId;

    QString receiverId = userId;
    emit signalDispatchFriendEvent(receiverId, jsonObj);
}

void ClientSocket::sendJsonToClient(QJsonObject &jsonObj)
{
    // 给每条消息创建uuid标识
    QUuid uuid = QUuid::createUuid();
    QString uuidStr = uuid.toString();
    jsonObj["uuid"] = uuidStr;

    QJsonDocument jsonDoc(jsonObj);
    QByteArray jsonByteArray = jsonDoc.toJson(QJsonDocument::Compact);

    int length = jsonByteArray.size();
    QByteArray header;
    header.append(reinterpret_cast<const char*>(&length), sizeof (int));

    qDebug() << "Sending data:" << jsonByteArray;

    if (g_client->state() == QAbstractSocket::ConnectedState)
    {
        g_client->write(header + jsonByteArray);
        g_client->flush();
    }
}

void ClientSocket::onReadyRead()
{
    static QByteArray buffer; // 数据缓存
    buffer.append(g_client->readAll());

    while(true)
    {
        if (static_cast<size_t>(buffer.size()) < sizeof (int))
            return; // 数据不足，等待接受数据

        size_t length = 0;
        memcpy(&length, buffer.constData(), sizeof(int));

        if (static_cast<size_t>(buffer.size()) < sizeof (int) + length)
            return;

        QByteArray jsonData = buffer.mid(sizeof (int), length);
        buffer.remove(0, sizeof (int) + length);

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

        if (parseError.error != QJsonParseError::NoError)
        {
            qDebug() << "Json Error:" << parseError.errorString();
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        int type = jsonObj["type"].toInt();

        // switch case中不能定义对象 注意break
        switch (type) {
        case EVENT_LOGIN_REQUEST:
            parseLoginResponse(jsonObj);
            break;
        case EVENT_GET_FRIENDLIST:
            parseGetFriendListResponse(jsonObj);
            break;
        case EVENT_LOGOUT_RESPONSE:
            parseLogoutUpdate(jsonObj);
            break;
        case EVENT_GET_USERPROFILE:
            parseGetUserProfile(jsonObj);
            break;
        case EVENT_SEND_MESSAGE:
            parseDispatchMsg(jsonObj);
            break;
        case EVENT_RECV_OFFLINEMESSAGES:
            getOfflineMessages();
            break;
        case EVENT_GET_GROUPLIST:
            parseGetGroupList(jsonObj);
            break;
        case EVENT_REQUEST_REGISTER:
            parseRegister(jsonObj);
            break;
        case EVENT_QUERY_USER:
            parseQueryUser(jsonObj);
            break;
        case EVENT_REQUEST_ADD_FRIEND:
            parseAddFriendRequest(jsonObj);
            break;
        case EVENT_DELETE_FRIENDGROUP:
            parseDeleteFriendGroup(jsonObj);
            break;
        case EVENT_ADD_FRIENDREMARK:
            parseUpdateFriendRemark(jsonObj);
            break;
        case EVENT_RENAME_GROUP:
            parseRenameGroup(jsonObj);
            break;
        case EVENT_CONFIRM_FRIENDREQUEST:
            parseFriendRequestStatus(jsonObj);
            break;
        case EVENT_QUERY_GROUP:
            parseQueryGroup(jsonObj);
            break;
        case EVENT_JOIN_GROUP:
            parseJoinGroup(jsonObj);
            break;
        case EVENT_GET_GROUPMEMBERS:
            parseQueryGroupMembers(jsonObj);
            break;
        case EVENT_CREATE_GROUP:
            parseCreateGroup(jsonObj);
            break;
        default:
            break;
        }
    }
}
