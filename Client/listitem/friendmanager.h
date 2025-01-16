#ifndef FRIENDMANAGER_H
#define FRIENDMANAGER_H

#include <QWidget>

class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QListWidget;
class QSpacerItem;
class User;
class Group;
class roundedLabel;

namespace Ui {
class FriendManager;
}

//////////////////////////////////////////////
/// \brief The FriendManager class
/// 好友管理，点击mainwindow中左边竖状菜单栏中人物按钮可显示
class FriendManager : public QWidget
{
    Q_OBJECT

public:
    explicit FriendManager(QWidget *parent = nullptr);
    ~FriendManager();

signals:
    void signalSwitchToChat(int chatType, const QString& userId);   // 切换到好友列表中好友的聊天窗口
    void signalSwitchToRequest();                                   // 切换到好友请求界面

private slots:
    void on_btnFriend_clicked();                                    // 显示好友列表
    void on_btnGroup_clicked();                                     // 显示群组列表
    void on_btnFriendMsg_clicked();                                 // 切换到相应的好友聊天窗口
    void on_btnGroupMsg_clicked();                                  // 切换到相应的群组聊天窗口

    void sltItemFriendClicked(QTreeWidgetItem *item);
    void sltItemGroupClicked(QTreeWidgetItem *item);

protected:
    void showEvent(QShowEvent *event) override;

private:
    Ui::FriendManager *ui;

    QPushButton *lastClickedButton;
    QWidget *btnBackground;
    QHash<QString, QTreeWidgetItem*> m_rootItem;
    QHash<QString, User> m_friendList;
    QHash<QString, Group> m_groupList;

    void initTree();                                                        // 初始化列表树
    void animateBackground(const QRect &targetGeometry);                    // 切换"好友""群聊"时两个按钮之间的动画
    void addSort(QTreeWidget* treeWidget, const QString& sortName);         // 添加分类
    void addSortItem(QTreeWidget* treeWidget, QTreeWidgetItem* rootItem);   // 添加分类Item
    void addFriendItem();                                                   // 添加好友Item
    void addGroupItem();                                                    // 添加群组Item

};

///////////////////////////////////////////////////////////////
/// \brief The RequestList class
/// 好友添加请求列表窗口
class RequestList : public QWidget
{
    Q_OBJECT

public:
    explicit RequestList(QWidget* parent = nullptr);
    ~RequestList();

    void addRequestItem(const User& user, const QDate& date, const QString& message, int status);   // 创建添加请求Item
    void showHistoryRequests(); // 展示历史好友添加消息
    //void updateConfirmStatus(QLabel* label, const int status);

signals:
    void signalAcceptRequest(const QVariant& info); // 接受好友请求

private slots:
    //void on_btnFilter_clicked();
    void on_btnClear_clicked(); // 清理好友添加请求列表

private:
    QVBoxLayout* vLayoutThis;
    QWidget *widgetTop;
    QHBoxLayout *hLayoutTop;
    QLabel *labelRequest;
    QSpacerItem *spacerTop;
    QPushButton *btnFilter;
    QPushButton *btnClear;
    QListWidget *listWidgetRequest;

    void setAvatar(roundedLabel* labelAvatar, const QString& avatarUUID);   // 设置头像，本地没有文件就到服务器下载
};

#endif // FRIENDMANAGER_H
