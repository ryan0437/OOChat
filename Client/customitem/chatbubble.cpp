#include "chatbubble.h"
#include "roundedlabel.h"
#include "filetransfer.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QSpacerItem>
#include <QDebug>
#include <sstream>
#include <QPushButton>
#include <QThread>
#include <QTimer>
#include <QDesktopServices>
#include <QFile>
#include <QDateTimeEdit>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTextBlock>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>

ChatBubble::ChatBubble(QWidget *parent) : QWidget(parent)
{
    this->setMinimumHeight(55);
    this->setMaximumSize(1100, 2000);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    this->setStyleSheet("border:none; background-color:transparent;");

    // 绘制头像区域
    widgetAvatar = new QWidget(this);
    vLayoutAvatar = new QVBoxLayout(widgetAvatar);
    labelAvatar = new roundedLabel;
    spacerAvatar = new QSpacerItem(50, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);

    labelAvatar->setFixedSize(50,50);
    labelAvatar->setAlignment(Qt::AlignCenter);

    vLayoutAvatar->setContentsMargins(0,0,0,0);
    vLayoutAvatar->setSpacing(0);

    vLayoutAvatar->addWidget(labelAvatar);
    vLayoutAvatar->addItem(spacerAvatar);

    widgetAvatar->setStyleSheet("background-color: transparent");

    // 绘制气泡区域（+群组显示的群友名称）
    widgetGroup = new QWidget(this);
    widgetGroup->setStyleSheet("background-color: transparent; border:none;");
    vLayoutGroup = new QVBoxLayout(widgetGroup);
    vLayoutGroup->setContentsMargins(0,0,0,0);
    vLayoutGroup->setSpacing(0);

    widgetBubble = new QWidget(this);
    vLayoutBubble = new QVBoxLayout(widgetBubble);

    vLayoutBubble->setSpacing(0);

    // 绘制主窗口
    hLayoutThis = new QHBoxLayout(this);
    widgetMain = new QWidget(this);
    hLayoutMain = new QHBoxLayout(widgetMain);
    widgetMain->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    hLayoutThis->setSpacing(0);
    hLayoutThis->setContentsMargins(0,0,0,0);
    hLayoutMain->setSpacing(10);
    hLayoutMain->setContentsMargins(0,0,0,0);

    hLayoutThis->addWidget(widgetMain);
}

void ChatBubble::setMessage(const ChatMessage &message, const QString &selfId)
{
    chatType = message.chatType;
    if (message.chatType == ChatTypes::GROUP)
    {
        QFont font("Microsoft YaHei");
        font.setPointSize(9);
        name = new QLabel(widgetGroup);
        name->setFont(font);

        name->setStyleSheet("color: rgb(155,155,155);");
        vLayoutGroup->addWidget(name);
        vLayoutGroup->setSpacing(5);
    }
    vLayoutBubble->setContentsMargins(15,10,0,5);
    vLayoutGroup->addWidget(widgetBubble);

    // 区分发送者和是否为群聊 改变头像摆放位置或者添加群中聊天者的名字
    if (message.sender == selfId)
    {
        if (message.chatType == ChatTypes::GROUP)
        {
            name->setText(selfId);
            vLayoutGroup->setAlignment(name, Qt::AlignRight);
        }
        hLayoutMain->addWidget(widgetGroup);
        hLayoutMain->addWidget(widgetAvatar);
        hLayoutThis->setAlignment(widgetMain, Qt::AlignRight);
    }else{
        if (message.chatType == ChatTypes::GROUP)
        {
            name->setText(message.sender);
            vLayoutGroup->setAlignment(name, Qt::AlignLeft);
        }
        hLayoutMain->addWidget(widgetAvatar);
        hLayoutMain->addWidget(widgetGroup);
        hLayoutThis->setAlignment(widgetMain, Qt::AlignLeft);
    }

    // 根据发送的消息类型设置具体的样式
    switch (message.type)
    {
    case ChatMessage::Text:
        setTextStyle(message, selfId);
        break;
    case ChatMessage::File:
        setFileStyle(message, selfId);
        break;
    }
}

void ChatBubble::setAvatar(const QString &avatarUUID)
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

void ChatBubble::setTextStyle(const ChatMessage &message, const QString &selfId)
{
    textEditMsg = new QTextEdit(widgetBubble);

    widgetBubble->setMaximumSize(880, 1980);
    widgetBubble->setMinimumSize(60,60);
    widgetBubble->setStyleSheet("background-color: rgb(0,153,255); border-radius:10px;border:none;");

    // 设置textEdit字体字号 微软雅黑11
    QFont font("Microsoft YaHei");
    font.setPointSize(11);
    textEditMsg->setFont(font);
    textEditMsg->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textEditMsg->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textEditMsg->setStyleSheet("background-color:rgb(0,153,255); color: white; border:none;");
    vLayoutBubble->addWidget(textEditMsg);

    if (message.sender != selfId)
    {
        widgetBubble->setStyleSheet("background-color: white; border-radius:10px; border:none;");
        textEditMsg->setStyleSheet("background-color:white; color: black; border:none;");
    }
    processMessageData(message.text);
    //textEditMsg->setHtml(message.text);
    //adjustBubbleSize(message);
    setAvatar(message.avatarUUID);
}

void ChatBubble::setFileStyle(const ChatMessage &message, const QString &selfId)
{
    lastTotalBytes = 0;
    widgetBubble->setFixedSize(QSize(340,130));
    widgetBubble->setStyleSheet("background-color: white; border-radius:10px ;border:none;");

    QFont font("Microsoft YaHei");
    font.setPointSize(9);
    QFont fontName = font;
    fontName.setPointSize(10);

    // 文件名+文件图标
    QWidget *widgetTop = new QWidget(widgetBubble);
    QLabel *labelFileName = new QLabel(widgetTop);
    QLabel *labelFileImg = new QLabel(widgetTop);
    QHBoxLayout *hLayoutTop = new QHBoxLayout(widgetTop);

    labelFileName->setText(message.fileName);
    labelFileName->setFont(fontName);
    labelFileImg->setPixmap(QPixmap(":/imgs/zip_file_icon.svg"));
    labelFileImg->setStyleSheet("border-radius:5px");
    labelFileImg->setScaledContents(true);  // 填充
    labelFileImg->setFixedSize(60,60);

    hLayoutTop->addWidget(labelFileName);
    hLayoutTop->addWidget(labelFileImg);
    hLayoutTop->setAlignment(labelFileName, Qt::AlignLeft | Qt::AlignTop);
    hLayoutTop->setAlignment(labelFileImg, Qt::AlignRight);

    // 将透明的PushButton覆盖在widgetBubble上面实现点击文件事件
    btnBubble = new QPushButton(widgetBubble);
    btnBubble->setStyleSheet("background: transparent;");
    btnBubble->setGeometry(widgetBubble->rect());
    //btnBubble->setEnabled(false);

    // 文件大小+文件发送或接收状态
    QWidget *widgetBottom = new QWidget(widgetBubble);
    QLabel *labelFileSize = new QLabel(widgetBottom);
    labelFileStatus = new QLabel(widgetBottom);
    QHBoxLayout *hLayoutBottom = new QHBoxLayout(widgetBottom);
    labelFileSize->setObjectName("labelFileSize"); // new方法创建控件对象时没有指定ObjectName，需要手动set
    labelFileStatus->setObjectName("labelFileStatus");

    labelFileSize->setText(formattedSize(message.fileSize));
    labelFileSize->setFont(font);
    labelFileSize->setMaximumWidth(80);
    labelFileSize->adjustSize();

    labelFileStatus->setFont(font);
    widgetBottom->setStyleSheet("QLabel#labelFileSize, QLabel#labelFileStatus{color:rgb(153,153,153)}");
    hLayoutBottom->addWidget(labelFileSize);
    hLayoutBottom->addWidget(labelFileStatus);
    hLayoutBottom->setAlignment(labelFileStatus, Qt::AlignLeft);

    vLayoutBubble->setContentsMargins(5,5,5,0);
    vLayoutBubble->addWidget(widgetTop);
    vLayoutBubble->addWidget(widgetBottom);

    // 设置文件状态
    if (isDownloadedFileExist(QString("./downloadedfile/" + message.fileName)))
        labelFileStatus->setText("已保存在下载目录");
    else if (message.sender != selfId)
        labelFileStatus->setText("未下载");
    else
        labelFileStatus->setText("已上传");

    setAvatar(message.avatarUUID);
    if (chatType == ChatTypes::GROUP)
        this->setFixedHeight(130 + name->height());
    else
        this->setFixedHeight(130);
    qDebug() << "大小：" << labelFileSize->size();

    connect(btnBubble, &QPushButton::clicked, this, [=](){
        if (message.sender == selfId)
        {
            btnBubble->setEnabled(false);
            disconnect(btnBubble, &QPushButton::clicked, this, nullptr);
            return;
        } else if (!isDownloadedFileExist(QString("./downloadedfile/" + message.fileName))){
            // 点击对方发来的文件聊天泡 开始下载文件
            QThread *workerThread = new QThread(this);  // 父对象设置为this，但是工作的线程不同，workerThread没有quit销毁的话，由this管理它的生命周期
            FileTransfer *worker = new FileTransfer();
            worker->moveToThread(workerThread);
            connect(worker, &FileTransfer::signalDownloadFinished, workerThread, &QThread::quit);
            connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
            connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

            worker->recvFile(message.fileUUID, 1);
            qDebug() << message.fileUUID;
            workerThread->start();

            // 显示下载速度
            QTimer *timer = new QTimer(this);
            connect(timer, &QTimer::timeout, this, [this, worker](){
                this->setTransferRate(worker->getBytesReceived(), worker->getTotalBytes());
            });

            connect(worker, &FileTransfer::signalDownloadFinished, this, [this, timer](){
                timer->stop();
                timer->deleteLater();
                this->labelFileStatus->setText("已下载");
                qDebug() << "file download finished...";
            });

            timer->start(1000);
        } else {
            // 在本地资源管理器中打开文件所在目录
            QUrl fileURL = QUrl::fromLocalFile("./downloadedfile");
            bool ok = QDesktopServices::openUrl(fileURL);
            if (!ok)
                qDebug() << "open local desktop services failed...";
        }
    });
}

void ChatBubble::setTransferRate(const qint64 &fileSize, const qint64 &totalBytes)
{
    if (fileSize >= totalBytes)
    {
        labelFileStatus->setText("已完成");
        emit signalTransferFinished();
        btnBubble->setEnabled(true);
        return;
    }
    QString transferRate = formattedSize(fileSize - lastTotalBytes) + "/s";
    labelFileStatus->setText(transferRate);
    lastTotalBytes = fileSize;
}

QString ChatBubble::formattedSize(const qint64 &fileSize)
{
    const double KB = 1024.0;
    const double MB = KB * 1024.0;
    const double GB = MB * 1024.0;
    double resSize = 0;
    QString unit;
    std::ostringstream oss;

    if (fileSize < KB)
        return QString::number(fileSize) + " B";
    else if (fileSize < MB){
        resSize = fileSize / KB;
        unit = " KB";
    }else if(fileSize < GB){
        resSize = fileSize / MB;
        unit = " MB";
    }else{
        resSize = fileSize / GB;
        unit = " GB";
    }

    QString formattedSize = QString::number(resSize, 'f', 1);   // 除了B之外的单位都保留一位小数
    return formattedSize + unit;
}

bool ChatBubble::isDownloadedFileExist(const QString& filePath)
{
    QFile file(filePath);
    if (file.exists())
    {
        qDebug() << "file exists:" << filePath;
        return true;
    }
    qDebug() << "file not exists";
    return false;
}

void ChatBubble::processMessageData(const QString &listStr)
{
    int left, top, right, bottom;   // 获取显示聊天信息的textEdit与widget之间的间隙大小
    QVBoxLayout* layout = dynamic_cast<QVBoxLayout*>(widgetBubble->layout());
    if (layout) {
        layout->getContentsMargins(&left, &top, &right, &bottom);
        qDebug() << "layout margin Left:" << left << "Top:" << top << "Right:" << right << "Bottom:" << bottom;
    }

    QTextCursor cursor = textEditMsg->textCursor();
    QList<QString> textList, imageList;
    int emptyLine = 0;

    QJsonDocument doc = QJsonDocument::fromJson(listStr.toUtf8());
    QJsonArray jsonArray = doc.array();

    for (QJsonArray::const_iterator it = jsonArray.begin(); it != jsonArray.end(); ++it) {
        QJsonObject jsonObj = it->toObject();
        QString content = jsonObj["content"].toString();
        QString type = jsonObj["type"].toString();

        cursor.movePosition(QTextCursor::End);

        if (it != jsonArray.begin())
            cursor.insertText("\n");    // 段落换行

        if (type == "text") {
            cursor.insertText(content);
            textList.append(content);
        } else if (type == "image") {
            // 所有的图片已经在ChatWidget中处理完毕，包括本地不存在，需要到服务器下载的图片
            imageList.append(content);
            QTextImageFormat imageFormat;
            imageFormat.setName(content);
            imageFormat.setVerticalAlignment(QTextCharFormat::AlignBaseline);   // 插入的图片与文本底部对齐
            cursor.insertImage(imageFormat);
        } else {
            emptyLine++;
            qDebug() << "empty line";
        }
    }
    textEditMsg->setTextCursor(cursor);

    // 获取输入的所有文字和图片尺寸
    QSize textTotalSize(0,0), imageTotalSize(0,0);
    if (!textList.isEmpty())
        textTotalSize = countTextSize(textList);
    if (!imageList.isEmpty())
        imageTotalSize = countImageSize(imageList);

    int emptyLineHeight = emptyLine * QFontMetrics(textEditMsg->font()).lineSpacing(); // 空段落高度
    int newWidth = qMax(textTotalSize.width(), imageTotalSize.width());
    int newHeight = qMax(widgetBubble->minimumHeight(), textTotalSize.height() + imageTotalSize.height() + top + bottom + emptyLineHeight);
    if (newHeight != widgetBubble->minimumHeight()) newHeight += top + bottom;

    widgetBubble->setFixedSize(QSize(newWidth, newHeight));
    if (chatType == ChatTypes::GROUP)
        this->setFixedHeight(newHeight + name->height());
    else
        this->setFixedHeight(newHeight);
    textEditMsg->setReadOnly(true);
}

QSize ChatBubble::countTextSize(QList<QString> strList)
{
    int maxWidth = widgetBubble->minimumWidth(), height = 0;
    QFontMetrics metrics(textEditMsg->font());
    int lineMaxWidth = widgetBubble->maximumWidth() - 40;
    int singleLineHeight = metrics.lineSpacing();

    for(const QString &text : strList) {
        int curWidth, curHeight;
        int strWidth = metrics.boundingRect(text).width();

        // 计算当前段落宽度curWidth
        if (strWidth + 40 <= widgetBubble->minimumWidth())
            curWidth = widgetBubble->minimumWidth();
        else if (strWidth + 40 >= widgetBubble->maximumWidth())
            curWidth = widgetBubble->maximumWidth();
        else
            curWidth = strWidth + 40;

        // 计算当前段落高度curHeight
        if (strWidth <= lineMaxWidth)
            curHeight = singleLineHeight;   // test widgetBubble->minimumHeight()
        else
        {
            int lineNum = strWidth / lineMaxWidth;
            if (lineNum * lineMaxWidth != strWidth) lineNum++;
            curHeight = lineNum * singleLineHeight;
        }

        maxWidth = qMax(maxWidth, curWidth);
        height += curHeight;
    }
    return QSize(maxWidth, height);
}

QSize ChatBubble::countImageSize(QList<QString> imageList)
{
    int maxWidth = 0, height = 0;
    for (const QString &imagePath : imageList) {
        QImage image(imagePath);

        if (!image.isNull()) {
            QSize imageSize = image.size();
            maxWidth = qMax(maxWidth, imageSize.width() + 40);
            height += imageSize.height();
        }
    }
    return QSize(maxWidth, height);
}

void ChatBubble::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿

    painter.setBrush(Qt::transparent);
    painter.setPen(Qt::NoPen);  // 无描边

    painter.drawRect(0, 0, width(), height());  // 绘制矩形，宽度和高度与Widget一样
}


TimeMessage::TimeMessage(const QDateTime &dateTime, QWidget *parent): QWidget(parent)
{
    hLayoutThis = new QHBoxLayout(this);
    labelTime = new QLabel(dateTime.toString("yyyy-MM-dd hh:mm"), this);

    hLayoutThis->addWidget(labelTime);
    hLayoutThis->setAlignment(labelTime, Qt::AlignCenter);
    hLayoutThis->setMargin(0);
    hLayoutThis->setSpacing(0);

    labelTime->setStyleSheet("color: rgb(160,160,160);"
                             "font-family: 'Microsoft YaHei';"
                             "font-size: 20px;");
    this->setStyleSheet("background: transparent;");
}
