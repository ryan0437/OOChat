#include "localdbmanager.h"
#include "clientsocket.h"
#include "structures.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QJsonDocument>

LocalDbManager::LocalDbManager(QObject *parent) : QObject(parent)
{
    m_selfId = ClientSocket::instance()->g_Id;
    QString dbName = m_selfId + "_database.db";
    db = QSqlDatabase::addDatabase("QSQLITE");

    // 如果数据库不存在则自动创建
    db.setDatabaseName(dbName);

    if (!db.open())
    {
        qDebug() << "数据库打开失败：" << db.lastError().text();
    } else {
        qDebug() << "数据库打开成功：" << dbName;
    }

    initDatabase();
}

LocalDbManager::~LocalDbManager()
{
    if (db.isOpen())
        db.close();
}

LocalDbManager &LocalDbManager::instance()
{
    static LocalDbManager instance;
    return instance;
}

void LocalDbManager::saveMessage(const ChatMessage &message)
{
    QString tableName;
    if (message.chatType == ChatTypes::USER)
        tableName = message.sender != m_selfId ? message.sender + "_table" : message.receiver + "_table";
    else
        tableName = message.receiver + "_table";

    QString createTable = QString("CREATE TABLE IF NOT EXISTS %1("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                "sender_id VARCHAR(16) NOT NULL,"
                                "receiver_id VARCHAR(16) NOT NULL,"
                                "message TEXT NOT NULL,"
                                "message_type INT NOT NULL,"
                                "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP)").arg(tableName);
    QSqlQuery query;
    bool isCreated = query.exec(createTable);
    if (isCreated)
        qDebug() << "table already exited:" << tableName;
    else
    {
        qDebug() << "table creation failed:" << query.lastError().text();
        return;
    }

    QString sqlInsert = QString("INSERT INTO %1 "
                                "(sender_id, receiver_id, message, message_type, timestamp) "
                                "VALUES (:sender_id, :receiver_id, :message, :message_type, :timestamp)").arg(tableName);
    query.prepare(sqlInsert);
    query.bindValue(":sender_id", message.sender);
    query.bindValue(":receiver_id", message.receiver);
    query.bindValue(":message_type", message.type);
    query.bindValue(":timestamp", message.time);
    if (message.chatType == ChatTypes::USER)
        query.bindValue(":message", message.text);
    else {
        QJsonObject groupJsonObj;
        groupJsonObj["senderAvatar"] = message.avatarUUID;
        groupJsonObj["message"] = message.text;
        QJsonDocument groupJsonDoc(groupJsonObj);
        QString groupMessageStr = groupJsonDoc.toJson(QJsonDocument::Compact);
        query.bindValue(":message", groupMessageStr);
    }

    if (query.exec())
    {
        qDebug() << "successfully saved messsage:" << message.text << " from " << message.sender;
    }
    else
    {
        qDebug() << "message save failed:" << query.lastError().text();
    }
}

QList<ChatMessage> LocalDbManager::getHistoryMessage(const QString& userOrGroupId, int chatType)
{
    QList<ChatMessage> historyList;
    QString tableName = userOrGroupId + "_table";
    if (chatType == ChatTypes::USER)
    {
        QString sqlQuery = QString("SELECT sender_id, receiver_id, message, message_type, timestamp FROM %1;").arg(tableName);
        QSqlQuery query;
        query.prepare(sqlQuery);

        if (query.exec())
        {
            while(query.next())
            {
                ChatMessage message;
                message.sender = query.value(0).toString();
                message.receiver = query.value(1).toString();
                message.type = static_cast<ChatMessage::MessageType>(query.value(3).toInt());
                message.time = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
                if (message.type == ChatMessage::Text)
                    message.text = query.value(2).toString();
                else if (message.type == ChatMessage::File)
                {
                    QString msg = query.value(2).toString();
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toUtf8());
                    if (jsonDoc.isObject())
                    {
                        QJsonObject fileObj = jsonDoc.object();
                        message.fileName = fileObj["fileName"].toString();
                        message.fileSize = fileObj["fileSize"].toVariant().toLongLong();
                        message.fileUUID = fileObj["fileUUID"].toString();
                    }else{
                        qDebug() << "parse failed...";
                    }
                }

                //message.time = query.value(4).toDateTime();
                historyList.append(message);
            }
        }
        else
        {
            qDebug() << "Query Failed: " << query.lastError().text();
        }
    }else{
        QString sqlQuery = QString("SELECT sender_id, receiver_id, message, message_type, timestamp FROM %1;").arg(tableName);
        QSqlQuery query;
        query.prepare(sqlQuery);

        if (query.exec())
        {
            while(query.next())
            {
                ChatMessage message;
                message.sender = query.value(0).toString();
                message.receiver = query.value(1).toString();
                message.type = static_cast<ChatMessage::MessageType>(query.value(3).toInt());
                message.time = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
                if (message.type == ChatMessage::Text) {
                    QString text = query.value(2).toString();
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(text.toUtf8());
                    if (jsonDoc.isNull()) {
                        qDebug() << "failed to parse json string...";
                        continue;
                    }
                    QJsonObject jsonObj = jsonDoc.object();
                    message.avatarUUID = jsonObj["senderAvatar"].toString();
                    message.text = jsonObj["message"].toString();
                }
                else if (message.type == ChatMessage::File)
                {
                    QString msg = query.value(2).toString();
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toUtf8());
                    if (jsonDoc.isObject())
                    {
                        QJsonObject fileObj = jsonDoc.object();
                        message.fileName = fileObj["fileName"].toString();
                        message.fileSize = fileObj["fileSize"].toVariant().toLongLong();
                        message.fileUUID = fileObj["fileUUID"].toString();
                    }else{
                        qDebug() << "parse failed...";
                    }
                }
                historyList.append(message);
            }
        }else{
            qDebug() << "Query Failed: " << query.lastError().text();
        }
    }
    return historyList;
}

ChatMessage LocalDbManager::getLastestMsg(const QString &userId)
{
    ChatMessage message;
    QString tableName = userId + "_table";
    QString sqlQuery = QString("SELECT message, timestamp FROM %1 ORDER BY id DESC LIMIT 1;").arg(tableName);
    QSqlQuery query;
    query.prepare(sqlQuery);

    if (query.exec() && query.next()) {
        QString text = query.value(0).toString();
        QString timeStr = query.value(1).toString();
        QDateTime time = QDateTime::fromString(timeStr, Qt::ISODate);
        message.text = text;
        message.time = time;
    }
    else
        qDebug() << "Query lastest msg failed: " << query.lastError().text();
    return message;
}

void LocalDbManager::saveRequest(const User &peer, const QString &msg, const QDate &date, int status)
{
    QString tableName = m_selfId + "_requestlist";
    int role = status == RequestStatus::WaitingForSelf ? 0 : 1;     // 0为接收方，1为发送方
    QSqlQuery query;
    QString sqlInsert = QString("INSERT INTO %1 "
                                "(user_id, peer_id, peer_name, peer_avatarUUID, message, status, role, timestamp) "
                                "VALUES (:user_id, :peer_id, :peer_name, :peer_avatarUUID, :message, :status, :role, :timestamp)").arg(tableName);
    query.prepare(sqlInsert);
    query.bindValue(":user_id", m_selfId);
    query.bindValue(":peer_id", peer.id);
    query.bindValue(":peer_name", peer.name);
    query.bindValue(":peer_avatarUUID", peer.avatarUUID);
    query.bindValue(":message", msg);
    query.bindValue(":status", status);
    query.bindValue(":role", role);
    query.bindValue(":timestamp", date.toString("yyyy-MM-dd"));

    if (query.exec()) {
        qDebug() << "save friend request successfully";
    } else {
        qDebug() << "save friend request failed" << query.lastError().text();
    }
}

void LocalDbManager::updateRequestStatus(const QString &userId, const QString &peerId, int newStatus)
{
    QSqlQuery query;
    QString tableName = m_selfId + "_requestlist";
    QString sqlUpdate = QString("UPDATE %1 "
                                "SET status = :new_status "
                                "WHERE user_id = :userId and peer_id = :peerId").arg(tableName);
    query.prepare(sqlUpdate);
    query.bindValue(":new_status", newStatus);
    query.bindValue(":userId", userId);
    query.bindValue(":peerId", peerId);

    if (query.exec()) {
        qDebug() << "update friend request status successfully" << userId << peerId << newStatus;
    } else {
        qDebug() << "update friend request status failed" << query.lastError().text();
    }
}


QList<QVariantMap> LocalDbManager::getHistoryRequest()
{
    QList<QVariantMap> requestList;

    QSqlQuery query;
    QString tableName = m_selfId + "_requestlist";
    QString queryStr = QString("SELECT * FROM %1").arg(tableName);

    if (query.exec(queryStr)) {
        while (query.next()) {
            QVariantMap request;
            request["userId"] = query.value("user_id");
            request["peerId"] = query.value("peer_id");
            request["peerName"] = query.value("peer_name");
            request["avatarUUID"] = query.value("peer_avatarUUID");
            request["message"] = query.value("message");
            request["status"] = query.value("status");
            request["role"] = query.value("role");
            request["date"] = query.value("timestamp");

            requestList.append(request);
        }
        qDebug() << "got history request num:" << requestList.size();
    } else {
        qDebug() << "failed to fetch history firend requests" << query.lastError().text();
    }
    return requestList;
}

void LocalDbManager::initDatabase()
{
    QSqlQuery query;
    QString create_requestlist_table = QString("CREATE TABLE IF NOT EXISTS %1_requestlist ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id VARCHAR(16) NOT NULL, "
        "peer_id VARCHAR(16) NOT NULL, "
        "peer_name VARCHAR(32) NOT NULL, "
        "peer_avatarUUID VARCHAR(256) NOT NULL, "
        "message TEXT NOT NULL, "
        "status INT NOT NULL, "
        "role INT NOT NULL,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP)"
    ).arg(m_selfId);

    if (!query.exec(create_requestlist_table)){
        qDebug() << "create requsetlist table failed" << query.lastError().text();
    } else {
        qDebug() << "create requestlist table successfully";
    }
}
