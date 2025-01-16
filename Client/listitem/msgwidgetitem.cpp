#include "msgwidgetitem.h"
#include "ui_msgwidgetitem.h"
#include "filetransfer.h"

#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QDateTime>

MsgWidgetItem::MsgWidgetItem(ChatTypes chatType, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MsgWidgetItem),
    m_chatType(chatType)
{
    ui->setupUi(this);
}

MsgWidgetItem::~MsgWidgetItem()
{
    delete ui;
}

void MsgWidgetItem::setInitItem(User user)
{
    ui->labelName->setText(user.name);
    setavatar(user.avatarUUID);
}

void MsgWidgetItem::setInitItem(Group group)
{
    ui->labelName->setText(group.name);

    setavatar(group.avatarUUID);
}

void MsgWidgetItem::setCurrentClickedStyle()
{
    ui->widgetRight->setStyleSheet(
        "QLabel { color: white }"
    );
}

void MsgWidgetItem::setPreviousClickedStyle()
{
    ui->widgetRight->setStyleSheet(
        "QLabel#labelLatestMsg, QLabel#labelTime {"
        "    color: rgb(153, 153, 153);"
        "}"
        "QLabel#labelStatus, QLabel#labelName {"
        "    color: black;"
        "}");
}

void MsgWidgetItem::setLatestMsg(const QString& latestMsg)
{
    if (latestMsg == "文件") {
        ui->labelLatestMsg->setText(latestMsg);
        return;
    }
    QString message;
    QList<QString> messageList;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(latestMsg.toUtf8());
    if (jsonDoc.isArray()) {
        QJsonArray jsonArray = jsonDoc.array();
        if (!jsonArray.isEmpty()) {
            QJsonValue firstValue = jsonArray.first();
            QJsonObject firstObj = firstValue.toObject();
            QString type = firstObj["type"].toString();
            if (type == "text")
                message = firstObj["content"].toString();
            else if (type == "image")
                message = "[图片]";
            else
                message = "";
        }
    } else if (jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.contains("fileName"))
            message = "[文件]";
        else
            message = "";
    } else
        message = latestMsg;

    ui->labelLatestMsg->setText(message);
}

void MsgWidgetItem::setName(const QString &newName)
{
    ui->labelName->setText(newName);
}

void MsgWidgetItem::setTime(const QDateTime &dateTime)
{
    ui->labelTime->setText(formattedTime(dateTime));
}

QString MsgWidgetItem::formattedTime(const QDateTime &dateTime)
{
    QDateTime currentDateTime = QDateTime::currentDateTime();

    // 今天：小时:分钟
    if (dateTime.date() == currentDateTime.date())
        return dateTime.toString("HH:mm");
    else if (dateTime.date() == currentDateTime.addDays(-1).date())
        return "昨天";

    // 本周内
    QDateTime startOfWeek = currentDateTime.addDays(-currentDateTime.date().dayOfWeek() + 1);   // 本周的周一日期
    QDateTime endOfWeek = startOfWeek.addDays(6);                                               // 本周的周日日期
    if (dateTime >= startOfWeek && dateTime <= endOfWeek)
        return dateTime.toString("dddd");   // dddd"星期x"，ddd"周x"

    // 其他时间
    return dateTime.toString("yyyy-MM-dd"); // 年月日
}

QString MsgWidgetItem::getCurName()
{
    return ui->labelName->text();
}

WidgetBadge *MsgWidgetItem::getWidgetBadge()
{
    return ui->unreadBadge;
}

void MsgWidgetItem::setavatar(const QString& avatarUUID)
{
    if (avatarUUID.startsWith(":/")) {
        ui->labelAvatar->setPixmap(avatarUUID);
        return;
    }

    QString avatarPath = "./chatpics/" + avatarUUID;
    if (!QFile::exists(avatarPath)) {
        qDebug() << "用户头像不存在，需要下载！";
        ResourceTransfer* resourceTransfer = new ResourceTransfer(this);
        resourceTransfer->downloadResource(avatarUUID);
        connect(resourceTransfer, &ResourceTransfer::signalResourceDownloaded, this, [=](const QString& avatarPath){
            ui->labelAvatar->setPixmap(avatarPath);
        });
    } else {
        ui->labelAvatar->setPixmap(avatarPath);
    }
}

