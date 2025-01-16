#include "dbmanager.h"
#include "structures.h"
#include "types.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>

DbManager::DbManager(){}

DbManager::~DbManager(){}

bool DbManager::checkUserCredential(const QString &UserId, const QString &UserPwd)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT 1 FROM userinfo "
                  "WHERE account = :UserId AND passwd = :UserPwd LIMIT 1");
    query.bindValue(":UserId", UserId);
    query.bindValue(":UserPwd", UserPwd);

    bool success = query.exec() && query.next();
    if (success)
        updateUserStatus(UserId, EVENT_LOGIN_RESPONSE);

    releaseConnection(db);
    return success;
}

// 获取用户好友列表
QList<User> DbManager::getFriendList(const QString &UserId)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    QList<User> friendList;
    // CASE WHEN <条件1> THEN <结果1> ELSE <结果2> END 好像有点问题。。。
    query.prepare("SELECT f.account2, u.name, u.status, u.avatarUUID "
                  "FROM friendinfo f "
                  "JOIN userinfo u ON f.account2 = u.account "
                  "WHERE f.account1 = :UserId");
    query.bindValue(":UserId", UserId);

    if (query.exec())
    {
        while (query.next())
        {
            QString friendId =  query.value(0).toString();
            QString friendName = query.value(1).toString();
            int friendStatus = query.value(2).toInt();
            QString friendavatar = query.value(3).toString();

            User friendUser(friendId, friendName, static_cast<Status>(friendStatus), friendavatar);

            friendList.append(friendUser);
        }
    }
    else
    {
        qDebug() << "Query failed: " << query.lastError().text();
    }

    if (friendList.isEmpty())
    {
        qDebug() << "no friends";
    }
    releaseConnection(db);
    return friendList;
}

void DbManager::updateUserStatus(const QString &UserId, int status)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE userinfo SET status = :status WHERE account = :UserId");
    query.bindValue(":UserId", UserId);
    status = status == EVENT_LOGIN_RESPONSE ? 1 : 0;    // 数据库中1为在线
    query.bindValue(":status", status);

    if (query.exec())
    {
        qDebug() << "set" << UserId << " status: " << status;
    }
    else
    {
        qDebug() << "Query Failed: " << query.lastError().text();
    }
    releaseConnection(db);
}

int DbManager::getUserStatus(const QString &UserId)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT status FROM userinfo WHERE account = :UserId");
    query.bindValue(":UserId", UserId);
    if (query.exec() && query.next())
    {
        releaseConnection(db);
        return query.value(0).toInt();
    }else
    {
        qDebug() << "Query failed: " << query.lastError().text();
        releaseConnection(db);
        return -1;
    }
}

User DbManager::getUserProfile(const QString &UserId)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT account, name, status, avatarUUID "
                  "FROM userinfo "
                  "WHERE account = :UserId");
    query.bindValue(":UserId", UserId);
    if (query.exec() && query.next())
    {
        QString id = query.value(0).toString();
        QString name = query.value(1).toString();
        int status = query.value(2).toInt();
        QString avatarUUID = query.value(3).toString();
        releaseConnection(db);
        return User(id, name, static_cast<Status>(status), avatarUUID);
    }
    releaseConnection(db);
    return User();
}

void DbManager::saveOfflineMessages(const QJsonObject &jsonObj)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    QString sender_id = jsonObj["sender"].toString();
    QString receiver_id = jsonObj["receiver"].toString();
    QString message = jsonObj["msg"].toString();
    int message_type = jsonObj["messageType"].toInt();
    QDateTime time = QDateTime::fromString(jsonObj["time"].toString(), Qt::ISODate);

    if (jsonObj["chatType"].toInt() == ChatTypes::USER)
    {
        query.prepare("INSERT INTO offline_messages_user "
                      "(sender_id, receiver_id, message, message_type, timestamp) "
                      "VALUES(:sender_id, :receiver_id, :message, :message_type, :timestamp);");
        query.bindValue(":sender_id", sender_id);
        query.bindValue(":receiver_id", receiver_id);
        query.bindValue(":message", message);
        query.bindValue(":message_type", message_type);
        query.bindValue(":timestamp", time);
    }
    else{
        QString group_id = jsonObj["recvGroup"].toString();
        query.prepare("INSERT INTO offline_messages_group "
                      "(group_id, sender_id, receiver_id, message, message_type, timestamp) "
                      "VALUES(:group_id, :sender_id, :receiver_id, :message, :message_type, :timestamp);");
        query.bindValue(":group_id", group_id);
        query.bindValue(":sender_id", sender_id);
        query.bindValue(":receiver_id", receiver_id);
        query.bindValue(":message", message);
        query.bindValue(":message_type", message_type);
        query.bindValue(":timestamp", time);
    }

    if (query.exec())
    {
        qDebug() << "saved offline message for receiver: " << receiver_id;
    }else{
        qDebug() << "Failed to save offline message:" << query.lastError().text();
    }
    releaseConnection(db);
}

QJsonArray DbManager::getOfflineMessages(const QString &userId)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    QJsonArray jsonArray;
    query.prepare("SELECT sender_id, receiver_id, message, message_type, timestamp "
                  "FROM offline_messages_user "
                  "WHERE receiver_id = :userId");
    query.bindValue(":userId", userId);
    if (query.exec())
    {
        while(query.next())
        {
            QJsonObject jsonObj;
            jsonObj["sender"] = query.value(0).toString();
            jsonObj["receiver"] = query.value(1).toString();
            jsonObj["message"] = query.value(2).toString();
            jsonObj["messageType"] = query.value(3).toInt();
            jsonObj["time"] = query.value(4).toDateTime().toString(Qt::ISODate);
            jsonObj["chatType"] = ChatTypes::USER;
            jsonArray.append(jsonObj);
        }
    }else{
        qDebug() << "failed to get offline messages for " << userId << ": " << query.lastError().text();
    }

    if (!jsonArray.isEmpty()) // 删除已经发送的离线接收消息
    {
        query.prepare("DELETE FROM offline_messages_user "
                      "WHERE receiver_id = :userId;");
        query.bindValue(":userId", userId);
        if (!query.exec())
            qDebug() << "failed to delete offline messages for " << userId << ": " << query.lastError().text();
    }

    query.prepare("SELECT group_id, sender_id, receiver_id, message, message_type, timestamp "
                  "FROM offline_messages_group "
                  "WHERE receiver_id = :userId");
    query.bindValue(":userId", userId);
    if (query.exec())
    {
        while(query.next())
        {
            QJsonObject jsonObj;
            jsonObj["recvGroup"] = query.value(0).toString();
            jsonObj["sender"] = query.value(1).toString();
            jsonObj["receiver"] = query.value(2).toString();
            jsonObj["messageType"] = query.value(4).toInt();
            jsonObj["time"] = query.value(5).toDateTime().toString(Qt::ISODate);
            jsonObj["chatType"] = ChatTypes::GROUP;

            QString jsonStr = query.value(3).toString();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonStr.toUtf8());
            if (jsonDoc.isObject())
            {
                QJsonObject msgJsonObj = jsonDoc.object();
                jsonObj["avatarUUID"] = msgJsonObj["senderAvatar"].toString();
                jsonObj["message"] = msgJsonObj["message"].toString();
            } else {
                 qDebug() << "parse failed...";
                 continue;
            }
            jsonArray.append(jsonObj);
        }
    }else{
        qDebug() << "failed to get group offline messages for " << userId << ": " << query.lastError().text();
    }

    if (!jsonArray.isEmpty()) // 删除已经发送的离线接收消息
    {
        query.prepare("DELETE FROM offline_messages_group WHERE receiver_id = :userId;");
        query.bindValue(":userId", userId);
        if (!query.exec())
            qDebug() << "failed to delete group offline messages for " << userId << ": " << query.lastError().text();
    }
    releaseConnection(db);
    return jsonArray;
}

QJsonArray DbManager::getGroupList(const QString &userId)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT gu.group_id, group_name, avatarUUID FROM groupinfo g "
                  "JOIN groupuserinfo gu ON g.group_id = gu.group_id "
                  "WHERE gu.user_id = :userId");
    query.bindValue(":userId", userId);
    QJsonArray jsonArray;
    if (query.exec())
    {
        while(query.next())
        {
            QJsonObject jsonObj;
            jsonObj["groupId"] = query.value(0).toString();
            jsonObj["groupName"] = query.value(1).toString();
            jsonObj["avatarUUID"] = query.value(2).toString();
            jsonArray.append(jsonObj);

            qDebug() << "find group for " << userId << ": " << query.value(0).toString() << query.value(1).toString() << query.value(2).toString();
        }
    }
    else
    {
        qDebug() << "failed to get group list for " << userId << query.lastError().text();
    }
    releaseConnection(db);
    return jsonArray;
}

QJsonArray DbManager::getGroupMembers(const QString &groupId)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT u.account, u.name, u.status, u.avatarUUID "
                  "FROM userinfo u "
                  "JOIN groupuserinfo gu ON gu.user_id = u.account "
                  "WHERE gu.group_id = :groupId");
    query.bindValue(":groupId", groupId);

    QJsonArray jsonArray;
    if (query.exec()) {
        while (query.next()) {
            QJsonObject jsonObj;
            jsonObj["id"] = query.value(0).toString();
            jsonObj["name"] = query.value(1).toString();
            jsonObj["status"] = query.value(2).toInt();
            jsonObj["avatarUUID"] = query.value(3).toString();
            jsonArray.append(jsonObj);
        }
        qDebug() << "got members num: " << jsonArray.size() << " for group" << groupId;
    } else
        qDebug() << "query group members failed for " << groupId << " " << query.lastError().text();

    releaseConnection(db);
    return jsonArray;
}

QList<QString> DbManager::getGroupALlUsers(const QString &groupId)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT u.account "
                  "FROM userinfo u "
                  "JOIN groupuserinfo gu ON u.account = gu.user_id "
                  "WHERE gu.group_id = :groupId;");
    query.bindValue(":groupId", groupId);
    QList<QString> groupUsers;
    if (query.exec())
    {
        while (query.next())
            groupUsers.append(query.value(0).toString());
    }
    else
    {
        qDebug() << "failed to get group users for " << groupId << query.lastError().text();
    }
    releaseConnection(db);
    return groupUsers;
}

void DbManager::saveFile(const ChatMessage &message)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("INSERT INTO filetransfer "
                  "(sender_id, receiver_id, file_name, file_uuid, file_path) "
                  "VALUES(:sender_id, :receiver_id, :file_name, :file_uuid, :file_path);");
    query.bindValue(":sender_id", message.sender);
    query.bindValue(":receiver_id", message.receiver);
    query.bindValue(":file_name", message.fileName);
    query.bindValue(":file_uuid", message.fileUUID);
    query.bindValue(":file_path", "./uploadedfile");

    if (!query.exec())
    {
        qDebug() << "save upload file failed:" << query.lastError().text();
    }
    qDebug() << "save uploaded file:" << message.fileName;
    releaseConnection(db);
}

ChatMessage DbManager::getFileInfo(const QString& fileUUID)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("SELECT sender_id, receiver_id, file_name, file_uuid, file_path "
                  "FROM filetransfer "
                  "WHERE file_uuid = :file_uuid;");
    query.bindValue(":file_uuid", fileUUID);

    ChatMessage message;
    if (query.exec())
    {
        if (query.next())
        {
            message.sender = query.value(0).toString();
            message.receiver = query.value(1).toString();
            message.fileName = query.value(2).toString();
            message.fileUUID = query.value(3).toString();
            message.filePath = query.value(4).toString();
            message.isVaild = true;
        }
    } else {
        qDebug() << "get file info failed:" << query.lastError().text();
    }
    releaseConnection(db);
    return message;
}

void DbManager::saveRegisterInfo(const Account &accountInfo)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("INSERT INTO userinfo "
                  "(account, passwd, name, avatarUUID) "
                  "VALUES(:account, :passwd, :name, :avatarUUID)");
    query.bindValue(":account", accountInfo.account);
    query.bindValue(":passwd", accountInfo.pwd);
    query.bindValue(":name", accountInfo.name);
    query.bindValue(":avatarUUID", accountInfo.avatar);

    if (query.exec())
        qDebug() << "successful insert register info...";
    else
        qDebug() << "insert register info failed:" << query.lastError().text();
    releaseConnection(db);
}

QJsonArray DbManager::getUserInfo(const QString &userId, int mode)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    if (mode == 1)
    {
        query.prepare("SELECT account, name, avatarUUID "
                      "FROM userinfo "
                      "WHERE account LIKE :userId");
        query.bindValue(":userId", "%" + userId + "%");
    } else {
        query.prepare("SELECT account, name, avatarUUID "
                      "FROM userinfo "
                      "WHERE account = :userId");
        query.bindValue(":userId", userId);
    }
    QJsonArray jsonArray;
    if (query.exec())
    {
        while(query.next())
        {
            QJsonObject jsonObj;
            jsonObj["userId"] = query.value(0).toString();
            jsonObj["name"] = query.value(1).toString();
            jsonObj["avatarUUID"] = query.value(2).toString();

            jsonArray.append(jsonObj);
        }
    } else
        qDebug() << "failed to get user info" << userId;
    qDebug() << "get user info nums for" << userId << " " << jsonArray.size();

    releaseConnection(db);
    return jsonArray;
}

QJsonArray DbManager::getGroupInfo(const QString &groupId, int mode)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    if (mode == 1)
    {
        query.prepare("SELECT group_id, group_name, avatarUUID "
                      "FROM groupinfo "
                      "WHERE group_id LIKE :groupId");
        query.bindValue(":groupId", "%" + groupId + "%");
    } else {
        query.prepare("SELECT group_id, group_name, avatarUUID "
                      "FROM groupinfo "
                      "WHERE group_id = :groupId");
        query.bindValue(":groupId", groupId);
    }
    QJsonArray jsonArray;
    if (query.exec())
    {
        while(query.next())
        {
            QJsonObject jsonObj;
            jsonObj["groupId"] = query.value(0).toString();
            jsonObj["groupName"] = query.value(1).toString();
            jsonObj["avatarUUID"] = query.value(2).toString();

            jsonArray.append(jsonObj);
        }
    } else
        qDebug() << "failed to get group info" << groupId;
    qDebug() << "get user info nums for" << groupId << " " << jsonArray.size();

    releaseConnection(db);
    return jsonArray;
}

void DbManager:: addFriendRelation(const QString &userA, const QString &userB)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("INSERT INTO friendinfo "
                  "(account1, account2) "
                  "VALUES(:account1, :account2)");
    query.bindValue(":account1", userA);
    query.bindValue(":account2", userB);
    if (query.exec())
        qDebug() << "successfully insertAB friend relation...";
    else
        qDebug() << "failed to insertAB friend relation " << query.lastError().text();

    query.bindValue(":account1", userB);
    query.bindValue(":account2", userA);
    if (query.exec())
        qDebug() << "successfully insertBA friend relation...";
    else
        qDebug() << "failed to insertBA friend relation " << query.lastError().text();
    releaseConnection(db);
}

void DbManager::addGroupRelation(const QString &groupId, const QString &userId)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("INSERT INTO groupuserinfo "
                  "(group_id, user_id) "
                  "VALUES (:groupId, :userId);");
    query.bindValue(":groupId", groupId);
    query.bindValue(":userId", userId);

    if (query.exec())
        qDebug() << "successfully insert group relation for" << userId;
    else
        qDebug() << "failed to insert group relation for" << userId << " " << query.lastError().text();
    releaseConnection(db);
}

void DbManager::deleteFriendGroup(const QString& idA, const QString& idB, int role)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    if (role == 0)
    {
        query.prepare("DELETE FROM friendinfo "
                      "WHERE (account1 = :idA AND account2 = :idB) "
                      "OR (account1 = :idB AND account2 = :idA)");
        query.bindValue(":idA", idA);
        query.bindValue(":idB", idB);
        if (query.exec()){
            qDebug() << "successfully delete relation:" << idA << " " << idB;
            deleteOfflineMsg(idA, idB, 0);
        } else
            qDebug() << "failed to detele friend relation " << query.lastError().text();

    } else if (role == 1) {
        query.prepare("DELETE FROM groupinfo WHERE group_id = :group_id");
        query.bindValue(":group_id", idB);
        if (query.exec()){
            qDebug() << "successfully delete group:" << idB;
            deleteOfflineMsg(idA, idB, 1);
        } else
            qDebug() << "failed to detele group " << query.lastError().text();
    }
    releaseConnection(db);
}

void DbManager::deleteOfflineMsg(const QString &senderId, const QString &receiverId, int role)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    QString deleteSql = QString("DELETE FROM %1 "
                                "WHERE (sender_id = :senderId AND receiver_id = :receiverId) "
                                "OR (sender_id = :receiverId AND receiver_id = :senderId)");

    if (role == 0)
        deleteSql = deleteSql.arg("offline_messages_user");
    else
        deleteSql = deleteSql.arg("offline_messages_group");

    query.prepare(deleteSql);
    query.bindValue(":senderId", senderId);
    query.bindValue(":receiverId", receiverId);

    if (query.exec())
        qDebug() << "successfully delete offline messages:" << senderId << " " << receiverId;
    else
        qDebug() << "failed to detele offline messages " << query.lastError().text();
    releaseConnection(db);
}

void DbManager::createGroup(const Group &group)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("INSERT INTO groupinfo "
                  "(group_id, group_name, avatarUUID) "
                  "VALUES(:groupId, :groupName, :avatarUUID);");
    query.bindValue(":groupId", group.id);
    query.bindValue(":groupName", group.name);
    query.bindValue(":avatarUUID", group.avatarUUID);

    if (query.exec())
        qDebug() << "successfully create group " << group.id;
    else
        qDebug() << "failed to create group " << query.lastError().text();
    releaseConnection(db);
}

void DbManager::quitGroup(const QString &groupId, const QString &userId)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("DELETE FROM groupuserinfo "
                  "WHERE group_id = :groupId AND user_id = :userId");
    query.bindValue(":groupId", groupId);
    query.bindValue(":userId", userId);

    if (query.exec())
        qDebug() << "successfully delete group relationship for: " << userId << groupId;
    else
        qDebug() << "failed delete group relationship" << query.lastError().text();
}

void DbManager::renameGroup(const QString &groupId, const QString &newName)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    query.prepare("UPDATE groupinfo SET group_name = :newName "
                  "WHERE group_id = :groupId");
    query.bindValue(":newName", newName);
    query.bindValue(":groupId", groupId);

    if (query.exec())
        qDebug() << "successfully update group name for:" << groupId << " " << newName;
    else
        qDebug() << "failed to  update group name " << query.lastError().text();
    releaseConnection(db);
}

void DbManager::saveOfflineEvent(const QString &userId, const QJsonObject &jsonObj)
{
   QJsonDocument doc(jsonObj);
   QString eventJson = doc.toJson(QJsonDocument::Compact);

   QSqlDatabase db = getConnection();
   QSqlQuery query(db);
   QString sqlInsert = QString("INSERT INTO offline_event (receiver_id, event_json) "
                               "VALUES (:receiver_id, :event_json);");

   query.prepare(sqlInsert);
   query.bindValue(":receiver_id", userId);  // 绑定接收者的 userId
   query.bindValue(":event_json", eventJson);  // 绑定事件的 JSON 数据

   if (query.exec()) {
       qDebug() << "Offline event saved successfully for user:" << userId;
   } else {
       qDebug() << "Failed to save offline event for user:" << userId << ", Error:" << query.lastError().text();
   }
   releaseConnection(db);
}

QList<QJsonObject> DbManager::getOfflineEvent(const QString &userId)
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);
    QList<QJsonObject> eventsList;
    QString sqlSelect = QString("SELECT event_json FROM offline_event WHERE receiver_id = :userId;");
    query.prepare(sqlSelect);
    query.bindValue(":userId", userId);

    if (query.exec()) {
        while (query.next()) {
            QString eventJsonStr = query.value(0).toString();

            QJsonDocument doc = QJsonDocument::fromJson(eventJsonStr.toUtf8());
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject jsonObj = doc.object();
                eventsList.append(jsonObj);
            } else {
                qDebug() << "Invalid JSON format in event_json for user:" << userId;
            }
        }
    } else {
        qDebug() << "Failed to retrieve offline events for user:" << userId << ", Error:" << query.lastError().text();
    }

    if (!eventsList.isEmpty()) {
        query.prepare("DELETE FROM offline_event "
                      "WHERE receiver_id = :userId;");
        query.bindValue(":userId", userId);
        if (!query.exec())
            qDebug() << "failed to delete offline event for " << userId;
    }

    releaseConnection(db);
    return eventsList;
}

QSqlDatabase DbManager::getConnection()
{
    return DatabasePool::instance().getConnection();
}

void DbManager::releaseConnection(QSqlDatabase db)
{
    DatabasePool::instance().releaseConnection(db);
}

///////////////////////////////////////////////////////////////////
/// \brief DatabasePool::instance 数据库连接池
/// \return
///
DatabasePool &DatabasePool::instance()
{
    static DatabasePool pool;
    return pool;
}

DatabasePool::~DatabasePool()
{
    foreach (QSqlDatabase connection, connectionQueue) {
        connection.close();
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
}

QSqlDatabase DatabasePool::getConnection()
{
    QMutexLocker locker(&mutex);
    while (connectionQueue.isEmpty()) {
        waitCondition.wait(&mutex);
    }
    return connectionQueue.dequeue();
}

void DatabasePool::releaseConnection(QSqlDatabase connection)
{
    QMutexLocker locker(&mutex);
    connectionQueue.enqueue(connection);
    waitCondition.wakeOne();
}

void DatabasePool::initDatabase()
{
    QSqlDatabase db = getConnection();
    QSqlQuery query(db);

    QString userinfo = QString("CREATE TABLE IF NOT EXISTS userinfo("
                               "account VARCHAR(16) NOT NULL PRIMARY KEY,"
                               "passwd VARCHAR(32) NOT NULL,"
                               "name VARCHAR(32) NOT NULL,"
                               "status INT NOT NULL DEFAULT 0,"
                               "registerData DATETIME DEFAULT CURRENT_TIMESTAMP,"
                               "avatarUUID VARCHAR(128)"
                               ");");
    if (!query.exec(userinfo))
        qDebug() << "create userinfo table failed:" << query.lastError().text();

    QString friendinfo = QString("CREATE TABLE IF NOT EXISTS friendinfo("
                                 "account1 VARCHAR(16) NOT NULL,"
                                 "account2 VARCHAR(16) NOT NULL"
                                 ");");
    if (!query.exec(friendinfo))
        qDebug() << "create friendinfo table failed:" << query.lastError().text();

    QString offlineEvent = QString("CREATE TABLE IF NOT EXISTS offline_event("
                                    "id INTEGER PRIMARY KEY AUTO_INCREMENT,"
                                    "receiver_id VARCHAR(16) NOT NULL, "
                                    "event_json TEXT NOT NULL"
                                    ");");
    if (!query.exec(offlineEvent))
        qDebug() << "create offline event table failed:" << query.lastError().text();

    QString offline_messages_user = QString("CREATE TABLE IF NOT EXISTS offline_messages_user("
                                           "id INTEGER PRIMARY KEY AUTO_INCREMENT,"     // sqlite语法为AUTOINCREMENT
                                           "sender_id VARCHAR(16) NOT NULL,"
                                           "receiver_id VARCHAR(16) NOT NULL,"
                                           "message TEXT NOT NULL,"
                                           "message_type INT NOT NULL,"
                                           "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
                                           ");");
    if (!query.exec(offline_messages_user))
        qDebug() << "create offline_messages_user table failed:" << query.lastError().text();

    QString offline_messages_group = QString("CREATE TABLE IF NOT EXISTS offline_messages_group("
                                            "id INTEGER PRIMARY KEY AUTO_INCREMENT,"
                                            "group_id VARCHAR(16) NOT NULL,"
                                            "sender_id VARCHAR(16) NOT NULL,"
                                            "receiver_id VARCHAR(16) NOT NULL,"
                                            "message TEXT NOT NULL,"
                                            "message_type INT NOT NULL,"
                                            "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
                                            ");");
    if (!query.exec(offline_messages_group))
        qDebug() << "create offline_messages_group table failed: " << query.lastError().text();

    QString groupinfo = QString("CREATE TABLE IF NOT EXISTS groupinfo("
                                "id INTEGER PRIMARY KEY AUTO_INCREMENT,"
                                "group_id VARCHAR(16) NOT NULL UNIQUE,"
                                "group_name VARCHAR(32) NOT NULL,"
                                "avatarUUID VARCHAR(128) NOT NULL"
                                ");");
    if (!query.exec(groupinfo))
        qDebug() << "create groupinfo table failed:" << query.lastError().text();

    QString groupuserinfo = QString("CREATE TABLE IF NOT EXISTS groupuserinfo("
                                    "id INTEGER PRIMARY KEY AUTO_INCREMENT,"
                                    "group_id VARCHAR(16) NOT NULL,"
                                    "user_id VARCHAR(16) NOT NULL,"
                                    "FOREIGN KEY (group_id) REFERENCES groupinfo(group_id) ON DELETE CASCADE"
                                    ");");
    if (!query.exec(groupuserinfo))
        qDebug() << "create groupuserinfo table failed:" << query.lastError().text();

    QString filetransfer = QString("CREATE TABLE IF NOT EXISTS filetransfer("
                                   "id INTEGER PRIMARY KEY AUTO_INCREMENT,"
                                   "sender_id VARCHAR(16) NOT NULL,"
                                   "receiver_id VARCHAR(16) NOT NULL,"
                                   "file_name VARCHAR(255) NOT NULL,"
                                   "file_uuid VARCHAR(255) NOT NULL,"
                                   "file_path TEXT NOT NULL,"
                                   "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
                                   ");");
    if (!query.exec(filetransfer))
        qDebug() << "create filetransfer table failed:" << query.lastError().text();

    releaseConnection(db);
}

DatabasePool::DatabasePool()
{
    databaseName = "chat";
    hostName = "localhost";
    userName = "root";
    password = "123456";
    port = 3306;
    maxConnections = 50;

    createConnection(maxConnections);
    initDatabase();
}

void DatabasePool::createConnection(int count)
{
    for (int i = 0; i < count ; ++i) {
        QString connectionName = QString("Connection-%1").arg(i);
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", connectionName);
        db.setDatabaseName(databaseName);
        db.setHostName(hostName);
        db.setPort(3306);
        db.setUserName(userName);
        db.setPassword(password);

        if (!db.open())
            qDebug() << "failed to open database connection:" << db.lastError().text();
        else
            connectionQueue.enqueue(db);
    }
}
