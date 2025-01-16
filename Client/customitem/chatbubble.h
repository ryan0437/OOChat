#ifndef CHATBUBBLE_H
#define CHATBUBBLE_H
#include "structures.h"

#include <QObject>
#include <QWidget>

class QHBoxLayout;
class QVBoxLayout;
class QTextEdit;
class QSpacerItem;
class roundedLabel;
class QLabel;
class QPushButton;
class QDateTimeEdit;

///////////////////////////////////////////////////////
/// \brief The ChatBubble class
/// 构造聊天气泡
///
class ChatBubble : public QWidget
{
    Q_OBJECT
public:
    explicit ChatBubble(QWidget *parent = nullptr);

    void setMessage(const ChatMessage& message, const QString& selfId);
    void setTransferRate(const qint64& fileSize, const qint64& totalBytes);


signals:
    void signalTransferFinished();
    void signalImgsProcessed();
    void signalBubbleProcessed();

private:
    // 本widget
    QHBoxLayout *hLayoutThis;
    QHBoxLayout *hLayoutMain;
    QWidget *widgetMain;

    // 头像widget
    QVBoxLayout *vLayoutAvatar;
    QWidget *widgetAvatar;
    QSpacerItem *spacerAvatar;
    roundedLabel *labelAvatar;

    // 气泡widget
    QVBoxLayout *vLayoutBubble;
    QTextEdit *textEditMsg;
    QWidget *widgetBubble;

    QWidget *widgetGroup;
    QVBoxLayout *vLayoutGroup;
    QLabel *name;

    // 文件样式设置
    QLabel *labelFileStatus;
    qint64 lastTotalBytes;
    QPushButton *btnBubble;

    ChatTypes chatType;

    void setAvatar(const QString& avatarPath);                                                          // 设置头像
    void setTextStyle(const ChatMessage &message, const QString &selfId);                               // 设置文本消息样式
    void setFileStyle(const ChatMessage &message, const QString &selfId);                               // 设置文件消息样式
    QString formattedSize(const qint64& fileSize);                                                      // 格式化文件大小 返回值带单位
    bool isDownloadedFileExist(const QString& filePath);                                                // 文件是否存在
    void processMessageData(const QString& listStr);                                                    // 预处理消息
    QSize countTextSize(QList<QString> strList);                                                        // 计算消息中文本的大小
    QSize countImageSize(QList<QString> imageList);                                                     // 计算消息中图片的大小

protected:
    void paintEvent(QPaintEvent *event) override;
};

/////////////////////////////////////////////////////////////
/// \brief TimeMessage::TimeMessage
/// 在ChatWIdget聊天窗口中添加时间 条件：两条消息聊天间隔大于三分钟或者为当前聊天第一条消息
/// \param parent
///
class TimeMessage : public QWidget
{
    Q_OBJECT

public:
    explicit TimeMessage(const QDateTime &dateTime, QWidget* parent = nullptr);

private:
    QHBoxLayout* hLayoutThis;
    QLabel* labelTime;
};

#endif // CHATBUBBLE_H
