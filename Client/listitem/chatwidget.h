#ifndef CHATWIDGET_H
#define CHATWIDGET_H
#include "structures.h"

#include <QWidget>

class MsgWidgetItem;
class ClientSocket;
class ChatBubble;
class roundedLabel;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
class QTextEdit;
class QLabel;
class QListWidget;

namespace Ui {
class ChatWidget;
}

///////////////////////////////////////////////////
/// \brief The ChatWidget class
/// 对话聊天窗口
class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    // 使用User构造
    explicit ChatWidget(const User& user, MsgWidgetItem* msgWidgetItem, QWidget *parent = nullptr);

    // 使用Group构造
    explicit ChatWidget(const Group& group, MsgWidgetItem* msgWidgetItem, QWidget* parent = nullptr);

    ~ChatWidget();

    ChatTypes chatType;                                     // 当前聊天的对象属性 用户单聊/群聊
    void addChatBubble(ChatMessage& message);               // 为消息添加ChatBubble
    void setName(const QString& newName);                   // 设置当前聊天窗的对象名字
    void sendInitMessage(const QString& helloMessage);      // 发送添加好友/加入群组初始信息 打招呼

signals:
    void signalShowLatestMsg(const QString& latestMsg);     // 在消息列表中展示最近的一条聊天信息

private slots:
    void sltTextChange();                                   // 检测消息输入框是否有文本 改变发送按钮样式
    void sltUpdateStatus(QString& sender, int status);      // 更新好友在线状态 在线/离线

    void on_toolBtnSend_clicked();                          // 点击发送文本消息按钮
    void on_btnFile_clicked();                              // 点击发送文件按钮
    void on_btnPic_clicked();                               // 点击插入图片

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void initUI();
    Ui::ChatWidget *ui;
    MsgWidgetItem* m_msgWidgetItem;     // 这个聊天对应的消息列表中的Item
    User m_userProfile;                 // 聊天对象User资料
    Group m_groupProfile;               // 群组资料

    ClientSocket* m_client;
    QString m_userId;                   // 对方ID
    QString m_selfId;                   // 自己ID
    QString m_selfAvatarPath;           // 自己的头像
    QDateTime m_lastTime;               // 上一条消息的时间

    void setStatusLabelStyle(int status);                                       // 设置在线/离线状态label样式 0:离线 1:在线
    ChatBubble *createChatBubble(const ChatMessage& message);                   // 创建聊天消息Bubble
    void addTimeMessage(QDateTime curTime);                                     // 添加时间消息
    QString contentList();                                                      // 分类提取消息
    QVariant createContentObject(const QString& content, const QString &type);  // 创建消息Object
    QList<QString> containsImageType(const QString& content);                   // 获取消息内的图片
};

///////////////////////////////////////////////////
/// \brief The GroupUI class
/// 群组UI 显示群公告（未实现目前只有按钮）、群组成员列表
class GroupUI : public QWidget
{
    Q_OBJECT

public:
    explicit GroupUI(QWidget *parent = nullptr);

    void addGroupMemberItem(const User user);
    void setAvatar(roundedLabel* labelAvatar, const QString& avatarUUID);

private:
    QVBoxLayout *vLayoutGroup;

    // 群公告区控件
    QWidget *widgetBulletin;
    QVBoxLayout *vLayoutBulletin;
    QPushButton *btnBulletin;
    QTextEdit *textEditBulletin;
    QHBoxLayout *hLayoutBtn;

    // 群公告按钮的Label
    QLabel *labelLeftBtn;
    QLabel *labelRightBtn;

    // 群组成员列表
    QWidget* widgetMembers;
    QVBoxLayout *vLayoutMembers;
    QLabel* labelMembers;
    QListWidget* listWidgetMembers;
};

////////////////////////////////////////
/// \brief The ProcessImages class
/// 处理消息中的图片资源，自己发送的图片上传到服务器，接收的图片到服务器下载
class ProcessImages : public QObject
{
    Q_OBJECT

public:
    ProcessImages(QObject *parent = nullptr) : QObject(parent),processedImgNum(0) {}

    void processImageResource(const QList<QString>& imageList, int command, const QString& m_selfId);

    enum IMAGES{
        DOWNLOAD_IMAGES = 1,    // 下载图片
        UPLOAD_IMAGES           // 上传图片
    };

signals:
    void signalImgsProcessed();

private:
    int processedImgNum;
};

#endif // CHATWIDGET_H
