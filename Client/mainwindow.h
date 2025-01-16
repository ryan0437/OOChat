#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "listitem/msglistview.h"
#include "listitem/chatwidget.h"
#include "listitem/friendmanager.h"
#include "customitem/framelesswidget.h"

#include <QTcpSocket>
#include <QMainWindow>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QListView>
#include <QToolButton>

class CustomMenu;
class AddFriendGroup;
class WidgetBadge;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

////////////////////////////////////////////
/// \brief The MainWindow class
/// 程序主界面
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void switchToMsgList();

private slots:
    void on_btnClose_clicked();                                                         // 窗口关闭按钮 点击
    void on_btnMini_clicked();                                                          // 窗口最小化按钮 点击
    void on_btnMax_clicked();                                                           // 窗口最大化/还原
    void on_toolButtonFriend_clicked();                                                 // 添加好友/群组按钮 点击
    void on_toolButtonMsg_clicked();                                                    // 消息界面 点击

    void sltSwitchChat(int chatType, const QString& userId);                            // 切换ChatWidget聊天窗口
    void sltSaveMsg(ChatMessage& message);                                              // 保存聊天信息到本地
    void sltGotFriendList();                                                            // 获取好友列表
    void sltGotGroupList();                                                             // 获取群组列表
    void sltAddFriendMenu();                                                            // 添加好友/群组菜单
    void sltCreateGroup();                                                              // 创建群组
    void sltAddFriendRequest(const User& user, const QString& msg, const QDate& date);  // 接收添加好友请求
    void sltSendFriendRequest(const User& user, const QString& msg, const QDate& date); // 发送好友请求
    void sltDeleteFriendGroup(const QString& id);                                       // 删除好友/解散群聊后删除窗口
    void sltDeletedRelation(const QString& id, int role);                               // 被删除好友/踢出群聊
    void sltShowNewFriendGroup(const QVariant& info);                                   // 消息列表中添加新好友item
    void sltUpdateRemark(const QString& userId, const QString& newName);                // 数据库中更新添加的好友备注
    void sltRenameGroup(const QString& groupId, const QString& newName);                // 修改Chatwidget中的群名
    void sltSwitchToPageBlank();                                                        // 切换到空白页面

private:
    Ui::MainWindow *ui;
    bool m_isResizing;
    bool m_isDragging;
    int m_resizeEdge;
    QPoint m_mouseStartPos;
    QPoint m_windowStartPos;
    QSize m_windowStartSize;

    QTcpSocket* m_client;                       // tcpsocket
    QString m_selfId;                           // 自己的ID
    CustomMenu *menuAddFriend;                  // 好友添加菜单
    MsgListView* m_msgListWidget;               // 好友消息列表
    FriendManager *m_friendManager;             // 好友管理界面
    QHash<QString, ChatWidget*> m_chatWidgetMap;// 键：好友/群组ID 值：对应的聊天窗口ChatWidget
    QHash<QString, int> m_unreadMessageCount;   // 键：好友/群组ID 值：对应的聊天未读消息条数
    RequestList* m_requestList;                 // 好友添加请求界面
    QDesktopWidget *m_desktop;                  // 本地计算机桌面对象
    WidgetBadge *unreadBadge;                   // 未读消息数量小控件

    bool isGotFriendList;       // 是否获取到好友列表
    bool isGotGroupList;        // 是否获取到群组列表

    void initUI();              // 初始化UI
    void setFriendGroupList();  // 设置好友/群组列表

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        // 记录鼠标的起始位置和窗口的位置和大小
        m_mouseStartPos = event->globalPos();
        m_windowStartPos = this->pos();
        m_windowStartSize = this->size();

        // 判断鼠标是否在窗口边缘
        int edgeRange = 10;  // 边缘的有效范围
        QRect rect = this->rect();

        if (event->x() <= edgeRange) {
            m_isResizing = true;
            m_resizeEdge = Qt::LeftEdge;
        } else if (event->x() >= rect.width() - edgeRange) {
            m_isResizing = true;
            m_resizeEdge = Qt::RightEdge;
        } else if (event->y() <= edgeRange) {
            m_isResizing = true;
            m_resizeEdge = Qt::TopEdge;
        } else if (event->y() >= rect.height() - edgeRange) {
            m_isResizing = true;
            m_resizeEdge = Qt::BottomEdge;
        } else {
            // 鼠标位于窗口内部时，可以拖动窗口
            m_isDragging = true;
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        // 判断是否正在调整窗口大小
        if (m_isResizing) {
            int dx = event->globalPos().x() - m_mouseStartPos.x();
            int dy = event->globalPos().y() - m_mouseStartPos.y();

            // 根据边缘来调整窗口大小
            if (m_resizeEdge == Qt::LeftEdge) {
                int newWidth = m_windowStartSize.width() - dx;
                if (newWidth > minimumWidth()) {
                    setGeometry(m_windowStartPos.x() + dx, m_windowStartPos.y(), newWidth, m_windowStartSize.height());
                }
            } else if (m_resizeEdge == Qt::RightEdge) {
                int newWidth = m_windowStartSize.width() + dx;
                if (newWidth > minimumWidth()) {
                    setGeometry(m_windowStartPos.x(), m_windowStartPos.y(), newWidth, m_windowStartSize.height());
                }
            } else if (m_resizeEdge == Qt::TopEdge) {
                int newHeight = m_windowStartSize.height() - dy;
                if (newHeight > minimumHeight()) {
                    setGeometry(m_windowStartPos.x(), m_windowStartPos.y() + dy, m_windowStartSize.width(), newHeight);
                }
            } else if (m_resizeEdge == Qt::BottomEdge) {
                int newHeight = m_windowStartSize.height() + dy;
                if (newHeight > minimumHeight()) {
                    setGeometry(m_windowStartPos.x(), m_windowStartPos.y(), m_windowStartSize.width(), newHeight);
                }
            }
        }
        // 判断是否在拖动窗口
        else if (m_isDragging) {
            int dx = event->globalPos().x() - m_mouseStartPos.x();
            int dy = event->globalPos().y() - m_mouseStartPos.y();
            move(m_windowStartPos.x() + dx, m_windowStartPos.y() + dy);
        }

        // 更新光标样式
        int edgeRange = 10;  // 边缘的有效范围
        QRect rect = this->rect();

        if (event->x() <= edgeRange) {
            setCursor(Qt::SizeHorCursor);  // 左边缘：水平调整大小
        } else if (event->x() >= rect.width() - edgeRange) {
            setCursor(Qt::SizeHorCursor);  // 右边缘：水平调整大小
        } else if (event->y() <= edgeRange) {
            setCursor(Qt::SizeVerCursor);  // 上边缘：垂直调整大小
        } else if (event->y() >= rect.height() - edgeRange) {
            setCursor(Qt::SizeVerCursor);  // 下边缘：垂直调整大小
        } else {
            setCursor(Qt::ArrowCursor);  // 否则显示箭头光标
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        Q_UNUSED(event);
        m_isResizing = false;
        m_isDragging = false;
    }
};

class DragWidgetFilter;

#endif // MAINWINDOW_H
