#ifndef MSGWIDGETITEM_H
#define MSGWIDGETITEM_H
#include "structures.h"
#include "chatwidget.h"

#include <QWidget>

class WidgetBadge;
namespace Ui {
class MsgWidgetItem;
}

///////////////////////////////////////////
/// \brief The MsgWidgetItem class
/// 构造聊天消息简化列表的Item
class MsgWidgetItem : public QWidget
{
    Q_OBJECT

public:
    explicit MsgWidgetItem(ChatTypes chatType, QWidget *parent = nullptr);
    ~MsgWidgetItem();

    void setInitItem(User user);
    void setInitItem(Group group);
    void setCurrentClickedStyle();                  // 更新当前被选中的ITEM样式
    void setPreviousClickedStyle();                 // 更新上一个被选中的样式
    void setLatestMsg(const QString& latestMsg);    // 更新最新的聊天信息
    void setName(const QString& newName);           // 设置昵称
    void setTime(const QDateTime& dateTime);        // 设置最新聊天信息的时间
    QString getCurName();                           // 获取当前昵称
    WidgetBadge* getWidgetBadge();                  // 获取未读消息badge对象

private:
    Ui::MsgWidgetItem *ui;
    ChatTypes m_chatType;

    void setavatar(const QString& avatarUUID);
    QString formattedTime(const QDateTime& time);   // 格式化QDateTime
};

#endif // MSGWIDGET_H
