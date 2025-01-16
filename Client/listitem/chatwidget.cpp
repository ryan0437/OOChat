#include "chatwidget.h"
#include "ui_chatwidget.h"
#include "clientsocket.h"
#include "customitem/chatbubble.h"
#include "customitem/roundedlabel.h"
#include "structures.h"
#include "localdbmanager.h"
#include "msgwidgetitem.h"
#include "filetransfer.h"

#include <QDebug>
#include <QKeyEvent>
#include <QScrollBar>
#include <QFile>
#include <QFileDialog>
#include <QThread>
#include <QTimer>
#include <QJsonDocument>
#include <QUuid>
#include <QPushButton>
#include <QTextBlock>
#include <QTextFrame>
#include <QTextImageFormat>
#include <QTextInlineObject>
#include <QTextFormat>
#include <QJsonArray>
#include <QMessageBox>

// 使用用户构造ChatWidget
ChatWidget::ChatWidget(const User& user, MsgWidgetItem* msgWidgetItem, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatWidget),
    m_msgWidgetItem(msgWidgetItem),
    m_userProfile(user)
{
    chatType = ChatTypes::USER;
    initUI();
    ui->labelName->setText(m_userProfile.name);

    qDebug() << "current widget USER:" << m_userProfile.id;
}

// 使用群组构造ChatWidget
ChatWidget::ChatWidget(const Group &group, MsgWidgetItem *msgWidgetItem, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ChatWidget),
    m_msgWidgetItem(msgWidgetItem),
    m_groupProfile(group)
{
    chatType = ChatTypes::GROUP;
    initUI();
    GroupUI *groupUI = new GroupUI(this);
    ui->hLayoutMain->addWidget(groupUI);
    ClientSocket::instance()->getGroupMember(m_groupProfile.id);
    ui->labelName->setText(m_groupProfile.name);

    qDebug() << "current widget GROUP:" << m_groupProfile.id;
}

ChatWidget::~ChatWidget()
{
    delete ui;
}

void ChatWidget::sltTextChange()
{
    if (ui->textEditMsg->toPlainText().isEmpty())
    {
        ui->toolBtnSend->setEnabled(false);
        ui->toolBtnSend->setStyleSheet("color: rgb(76,183,255);");
    }
    else
    {
        ui->toolBtnSend->setEnabled(true);
        ui->toolBtnSend->setStyleSheet("color: white");
    }
}

void ChatWidget::sltUpdateStatus(QString& sender, int status)
{
    if (sender != m_userId) return;
    setStatusLabelStyle(status);
}

void ChatWidget::on_toolBtnSend_clicked()
{
    QString text = contentList();
    ui->textEditMsg->clear();
    QString receiver = chatType == ChatTypes::USER ? m_userProfile.id : m_groupProfile.id;

    ChatMessage message;
    message.type = ChatMessage::Text;
    message.sender = m_selfId;
    message.receiver = receiver;
    message.chatType = chatType;
    message.text = text;
    message.time = QDateTime::currentDateTime();
    addChatBubble(message);

    QList<QString> imageList = containsImageType(message.text);
    if (imageList.isEmpty()) {
        ClientSocket::instance()->sendMsg(message);
        LocalDbManager::instance().saveMessage(message);
    } else {
        ProcessImages *processImage = new ProcessImages(this);
        processImage->processImageResource(imageList, ProcessImages::IMAGES::UPLOAD_IMAGES, m_selfId);
        connect(processImage, &ProcessImages::signalImgsProcessed, this, [=](){
            QTimer::singleShot(1500, this, [=]() {  // 延迟1.5秒发送，否则接收方时序不匹配。。。
                ClientSocket::instance()->sendMsg(message);
                LocalDbManager::instance().saveMessage(message);
            });
        });
    }

}

bool ChatWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->textEditMsg && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        {
            if (keyEvent->modifiers() == Qt::ShiftModifier) {
                QTextCursor cursor = ui->textEditMsg->textCursor();
                cursor.insertBlock();
            } else {
                emit ui->toolBtnSend->click();
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ChatWidget::initUI()
{
    ui->setupUi(this);
    ui->textEditMsg->installEventFilter(this); // 安装事件过滤器
    //ui->listWidgetMsg->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidgetMsg->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->listWidgetMsg->setSpacing(20);
    //ui->listWidgetMsg->setSelectionMode(QAbstractItemView::NoSelection);    // 无法选中（屏蔽悬浮时的背景 也可样式设置）
    ui->listWidgetMsg->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);    // 滚动条平滑滚动
    ui->listWidgetMsg->verticalScrollBar()->setSingleStep(10);

    m_client = ClientSocket::instance();
    m_selfId = m_client->g_Id;
    m_selfAvatarPath = m_client->g_Profile.avatarUUID;

    m_userId = chatType == ChatTypes::USER ? m_userProfile.id : m_groupProfile.id;
    if (chatType == ChatTypes::USER) {  // 只有用户单聊才有在线/离线标识符
        setStatusLabelStyle(m_userProfile.status);
        connect(m_client, &ClientSocket::signalUpdateStatus, this, &ChatWidget::sltUpdateStatus);
    }

    QList<ChatMessage> historyMessageList = LocalDbManager::instance().getHistoryMessage(m_userId, chatType);
    for(ChatMessage& message : historyMessageList)
    {
        message.chatType = chatType;
        addChatBubble(message);
    }

    connect(ui->textEditMsg, &QTextEdit::textChanged, this, &ChatWidget::sltTextChange);
}

void ChatWidget::setStatusLabelStyle(int status)
{
    // 在线：绿色， 离线：灰色
    if (status == 0) {
        ui->labelStatus->setStyleSheet("QLabel{"
                                       "border-radius: 10px;"
                                       "background-color: rgb(188,190,200);"
                                       "width: 20px;"
                                       "height: 20px;"
                                       "}");
    } else if (status == 1) {
        ui->labelStatus->setStyleSheet("QLabel{"
                                       "border-radius: 10px;"
                                       "background-color: rgb(31,227,136);"
                                       "width: 20px;"
                                       "height: 20px;"
                                       "}");
    }
}

ChatBubble *ChatWidget::createChatBubble(const ChatMessage &recvMessage)
{
    ChatMessage message = recvMessage;
    addTimeMessage(message.time);
    message.chatType = chatType;

    // 设置头像
    if (message.chatType == ChatTypes::USER) {
        if (message.sender != m_selfId)
            message.avatarUUID = m_userProfile.avatarUUID;
        else
            message.avatarUUID = m_selfAvatarPath;
    } else if (message.sender == m_selfId) {
        message.avatarUUID = m_selfAvatarPath;
    }

    // 创建并添加聊天气泡
    ChatBubble *bubble = new ChatBubble(this);

    // 使用 QTimer 在3秒后调用 setMessage
    bubble->setMessage(message, m_selfId);

    // 创建并添加聊天气泡
    QListWidgetItem *item = new QListWidgetItem(ui->listWidgetMsg);
    item->setSizeHint(bubble->size());
    item->setFlags(Qt::NoItemFlags);
    ui->listWidgetMsg->setItemWidget(item, bubble);
    ui->listWidgetMsg->scrollToBottom();

    // 更新最新消息
    m_msgWidgetItem->setTime(message.time);
    if (message.type == ChatMessage::Text)
        m_msgWidgetItem->setLatestMsg(message.text);
    else if (message.type == ChatMessage::File)
        m_msgWidgetItem->setLatestMsg("[文件]");

    return bubble;
}

void ChatWidget::addTimeMessage(QDateTime curTime)
{
    if (!m_lastTime.isValid() || m_lastTime.secsTo(curTime) >= 180) {
        TimeMessage *timeMessage = new TimeMessage(curTime, this);
        QListWidgetItem *item = new QListWidgetItem(ui->listWidgetMsg);
        item->setSizeHint(QSize(timeMessage->sizeHint().width(), 30));
        item->setFlags(Qt::NoItemFlags);

        ui->listWidgetMsg->setItemWidget(item, timeMessage);
    }
    m_lastTime = curTime;
}

QString ChatWidget::contentList()
{
    QList<QVariant> list;
    QTextDocument *document = ui->textEditMsg->document();

    // 遍历block
    for (QTextBlock block = document->begin(); block != document->end(); block = block.next()) {
        QString blockText = block.text();

        // 遍历块中的fragment
        bool isBlockEmpty = true;
        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            isBlockEmpty = false;
            QTextFragment fragment = it.fragment();

            if (fragment.isValid()) {
                if (fragment.charFormat().isImageFormat()) {
                    QTextImageFormat imgFormat = fragment.charFormat().toImageFormat();
                    if (!imgFormat.name().isEmpty()) {
                        QString imgURL = imgFormat.name();
                        list.append(createContentObject(imgURL, "image"));
                        qDebug() << "Img: " << imgURL;
                    }
                }
                else if (fragment.charFormat().isCharFormat())
                {
                    QString text = fragment.text().toUtf8();
                    list.append(createContentObject(text, "text"));
                    qDebug() << "Text: " << text;
                }
            }
        }
        if (isBlockEmpty) list.append(createContentObject(QString("\n"), "emptyLine"));   // 空段落
    }

    QJsonArray jsonArray;
    for(const QVariant &item : list) {
        if (item.isValid() && item.canConvert<QJsonObject>()) {
            QJsonObject jsonObj = item.toJsonObject();
            jsonArray.append(jsonObj);
        }
    }

    QJsonDocument jsonDoc(jsonArray);
    QString jsonString = jsonDoc.toJson(QJsonDocument::Compact);
    qDebug() << jsonString;
    return jsonString;
}

QVariant ChatWidget::createContentObject(const QString &content, const QString &type)
{
    QJsonObject jsonObj;
    jsonObj["content"] = content;
    jsonObj["type"] = type;
    return QVariant::fromValue(jsonObj);
}

QList<QString> ChatWidget::containsImageType(const QString &content)
{
    QList<QString> imageList;
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    QJsonArray jsonArray = doc.array();
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

void ChatWidget::addChatBubble(ChatMessage &message)
{
    QList<QString> imageList = containsImageType(message.text);
    if (imageList.isEmpty() || message.sender == m_selfId)
        createChatBubble(message);
    else {
        ProcessImages *processImages = new ProcessImages(this);
        connect(processImages, &ProcessImages::signalImgsProcessed, this, [=](){
           createChatBubble(message);
        });
        int command = ProcessImages::IMAGES::DOWNLOAD_IMAGES;
        processImages->processImageResource(imageList, command, m_selfId);
    }
}

void ChatWidget::setName(const QString &newName)
{
    ui->labelName->setText(newName);
}

void ChatWidget::sendInitMessage(const QString& helloMessage)
{
    ChatMessage message;
    message.type = ChatMessage::Text;
    message.sender = m_selfId;
    message.receiver = chatType == ChatTypes::USER ? m_userProfile.id : m_groupProfile.id;
    message.chatType = chatType == ChatTypes::USER ? ChatTypes::USER : ChatTypes::GROUP;
    message.text = helloMessage;
    message.time = QDateTime::currentDateTime();
    addChatBubble(message);
    ClientSocket::instance()->sendMsg(message);
    LocalDbManager::instance().saveMessage(message);
}

void ChatWidget::on_btnFile_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "select a file"); // 可选所有文件"ALL Files (*.*)"
    QFileInfo fileInfo(filePath);
    if (filePath.isEmpty())
    {
        qDebug() << "no file selected...";
        return;
    }
    else
        qDebug() << "selected file path:" << filePath;

    QThread *workerThread = new QThread(this);
    FileTransfer *worker = new FileTransfer();
    worker->moveToThread(workerThread);
    connect(worker, &FileTransfer::signalTransferFinished, workerThread, &QThread::quit);
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    ChatMessage message;
    message.type = ChatMessage::File;
    message.chatType = chatType;
    message.sender = m_selfId;
    message.receiver = m_userId;
    message.time = QDateTime::currentDateTime();
    message.fileName = fileInfo.fileName();
    message.filePath = filePath;
    message.fileSize = fileInfo.size();
    message.fileUUID = QUuid::createUuid().toString(QUuid::WithoutBraces);  // 生成文件对应的UUID

    worker->sendFile(message); // 在子线程中的sendFIle函数里将sltSendFile函数添加到子线程的工作队列中
    workerThread->start();


    ChatBubble *chatBubble = createChatBubble(message);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [chatBubble, worker]() {
        chatBubble->setTransferRate(worker->getBytesSent(), worker->getTotalBytes());
    });

    // 传输完成时停止定时器 使用FileTransfer::signalTransferFinished作为信号有时接收不到？？？暂未找到原因
    connect(chatBubble, &ChatBubble::signalTransferFinished, this, [=]() {
        timer->stop();
        timer->deleteLater();

        ChatMessage messageCopy = message;
        QJsonObject jsonObj;
        jsonObj["fileName"] = messageCopy.fileName;
        jsonObj["fileSize"] = messageCopy.fileSize;
        jsonObj["fileUUID"] = messageCopy.fileUUID;
        if (messageCopy.sender == m_selfId)
            jsonObj["isSent"] = true;
        QJsonDocument jsonDoc(jsonObj);
        QString jsonStr = jsonDoc.toJson(QJsonDocument::Compact);
        messageCopy.text = jsonStr;

        ClientSocket::instance()->sendMsg(messageCopy);
        LocalDbManager::instance().saveMessage(messageCopy);
        qDebug() << "文件传输完毕！";
    });

    timer->start(1000); // 计算每秒传输速率
}

/////////////////////////////////////////////////////////////////////////////////////////
/// \brief GroupUI::GroupUI 添加群组的UI，群组用户栏、公告栏
/// \param parent
///
GroupUI::GroupUI(QWidget *parent) : QWidget(parent)
{
    this->setObjectName("widgetMain");
    vLayoutGroup = new QVBoxLayout(this);

    widgetBulletin = new QWidget(this); // 群公告 未做具体功能
    vLayoutBulletin = new QVBoxLayout(widgetBulletin);
    btnBulletin = new QPushButton(widgetBulletin);
    textEditBulletin = new QTextEdit(widgetBulletin);

    hLayoutBtn = new QHBoxLayout(btnBulletin);
    labelLeftBtn = new QLabel("群公告", btnBulletin);
    labelRightBtn = new QLabel(">", btnBulletin);
    hLayoutBtn->addWidget(labelLeftBtn);
    hLayoutBtn->addStretch();
    hLayoutBtn->addWidget(labelRightBtn);

    vLayoutBulletin->addWidget(btnBulletin);
    vLayoutBulletin->addWidget(textEditBulletin);

    widgetMembers = new QWidget(this); // 群成员列表
    vLayoutMembers = new QVBoxLayout(widgetMembers);
    labelMembers = new QLabel("群聊成员", widgetMembers);
    listWidgetMembers = new QListWidget(widgetMembers);

    vLayoutMembers->addWidget(labelMembers);
    vLayoutMembers->addWidget(listWidgetMembers);

    vLayoutGroup->addWidget(widgetBulletin);
    vLayoutGroup->addWidget(widgetMembers);

    vLayoutGroup->setSpacing(0);
    vLayoutGroup->setContentsMargins(0,0,0,0);
    vLayoutBulletin->setSpacing(0);
    vLayoutBulletin->setContentsMargins(0,0,0,0);
    vLayoutMembers->setSpacing(0);
    vLayoutMembers->setContentsMargins(10,0,0,0);

    this->setMaximumWidth(250);
    btnBulletin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    btnBulletin->setFixedHeight(60);
    textEditBulletin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    labelMembers->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    labelMembers->setFixedHeight(60);
    listWidgetMembers->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    btnBulletin->setStyleSheet("background:transparent;"
                               "border:none;"
                               "font-family: 'Microsoft Yahei';"
                               "font-size: 10pt");
    labelMembers->setStyleSheet("font-family: 'Microsoft Yahei';"
                                "font-size: 10pt");
    textEditBulletin->setStyleSheet("background:transparent;"
                                    "border:none;"
                                    "border-bottom: 2px solid rgb(235,235,235);"
                                    "border-top: 2px solid rgb(235,235,235);");
    listWidgetMembers->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listWidgetMembers->setStyleSheet(
        "border:none;"
        "QListWidget::item {"
        "border:none;"
        "}"
        "QScrollBar:vertical {"
        "background-color: transparent;"
        "width: 10px;"
        "margin: 0px;"
        "border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical {"
        "background-color: rgb(215,215,215);"
        "border-radius: 5px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "height: 0px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "background: transparent;"
        "}");
    connect(ClientSocket::instance(), &ClientSocket::signalGotGroupMembers, this, [=](const QList<User> memberList){
        disconnect(ClientSocket::instance(), &ClientSocket::signalGotGroupMembers, this, nullptr);
        for(const User user : memberList) {
           addGroupMemberItem(user);
       }
    });
}

void GroupUI::addGroupMemberItem(const User user)
{
    QWidget* widgetMember = new QWidget(this);
    QHBoxLayout *hLayoutMember = new QHBoxLayout(widgetMember);
    roundedLabel *labelAvatar = new roundedLabel(widgetMember);
    QLabel *labelName = new QLabel(user.name, widgetMember);

    hLayoutMember->addWidget(labelAvatar);
    hLayoutMember->addWidget(labelName);

    labelAvatar->setFixedSize(30, 30);
    setAvatar(labelAvatar, user.avatarUUID);
    labelName->setStyleSheet("font-family: 'Microsoft YaHei';"
                             "font-size: 8pt;"
                             "color: rgb(162,162,162);");
    QListWidgetItem *item = new QListWidgetItem(listWidgetMembers);
    item->setSizeHint(QSize(widgetMember->sizeHint().width(), 50));
    item->setFlags(Qt::NoItemFlags);
    listWidgetMembers->addItem(item);
    listWidgetMembers->setItemWidget(item, widgetMember);
}

void GroupUI::setAvatar(roundedLabel* labelAvatar, const QString &avatarUUID)
{
    QString avatarPath = "./chatpics/" + avatarUUID;
    if (!QFile::exists(avatarPath)) {
        qDebug() << "用户头像不存在，需要下载！";
        ResourceTransfer* resourceTransfer = new ResourceTransfer(this);
        resourceTransfer->downloadResource(avatarUUID);
        connect(resourceTransfer, &ResourceTransfer::signalResourceDownloaded, this, [=](const QString& avatarPath){
            labelAvatar->setPixmap(avatarPath);
        });
    } else {
        labelAvatar->setPixmap(avatarPath);
    }
}

void ProcessImages::processImageResource(const QList<QString> &imageList, int command, const QString &m_selfId)
{
    if (command == IMAGES::DOWNLOAD_IMAGES) {
        for (const QString &imagePath : imageList) {
            if (!QFile::exists(imagePath)) {
                qDebug() << "聊天图片不存在，需要下载！";
                QString fileName = imagePath.section('/', -1);
                ResourceTransfer *resourceTransfer = new ResourceTransfer(this);
                resourceTransfer->downloadResource(fileName);
                connect(resourceTransfer, &ResourceTransfer::signalResourceDownloaded, this, [=]() mutable{
                    processedImgNum++;
                    if (processedImgNum == imageList.size())
                        emit signalImgsProcessed();
                });
            } else {
                processedImgNum++;
                if (processedImgNum == imageList.size())
                    emit signalImgsProcessed();
            }
        }
    } else if (command == IMAGES::UPLOAD_IMAGES) {
        ChatMessage message;
        message.type = ChatMessage::File;
        message.sender = m_selfId;
        message.receiver = "聊天图片";
        for (const QString& imagePath : imageList) {
            QString fileName = imagePath.section('/', -1);
            QFileInfo fileInfo(fileName);
            message.fileName = fileInfo.fileName();
            message.filePath = imagePath;
            message.fileSize = fileInfo.size();
            message.fileUUID = fileName;

            ResourceTransfer *resourceTransfer = new ResourceTransfer(this);
            resourceTransfer->uploadResource(message);
            connect(resourceTransfer, &ResourceTransfer::signalUploadFinished, this, [=]() mutable{
                processedImgNum++;
                if (processedImgNum == imageList.size())
                    emit signalImgsProcessed();
            });
        }
    }
}

void ChatWidget::on_btnPic_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "select a image",  "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (filePath.isEmpty()) return;

    QImage image(filePath);
    if (image.isNull()) {
        QMessageBox::warning(this, "invalid image", "选择的图片无效");
        return;
    }
    QSize maxSize(250,250);
    QSize imageSize = image.size();
    if (imageSize.width() > maxSize.width() || imageSize.height() > maxSize.height()) {
        image = image.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QString imgUUID = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString saveImgPath = "./chatpics/";
    QDir dir(saveImgPath);
    if (!dir.exists() && !dir.mkdir("."))
    {
        qDebug() << "make file path failed...";
        return;
    }
    image.save(saveImgPath + imgUUID + ".jpg");

    QTextCursor cursor = ui->textEditMsg->textCursor();
       cursor.insertImage(saveImgPath + imgUUID + ".jpg");
}
