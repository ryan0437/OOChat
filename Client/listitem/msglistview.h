#ifndef MSGLISTVIEW_H
#define MSGLISTVIEW_H

#include "structures.h"
#include "clientsocket.h"

#include <QWidget>

class QTcpSocket;
class QListView;
class QListWidgetItem;
class QStandardItem;
class QStandardItemModel;
class MsgWidgetItem;

namespace Ui {
class MsgListView;
}

//////////////////////////////////////////////
/// \brief The MsgListView class
/// 聊天消息简化列表
/// 显示好友/群组的头像、名称、最近一条聊天消息和相应的时间
class MsgListView : public QWidget
{
    Q_OBJECT

public:
    explicit MsgListView(QWidget *parent = nullptr);
    ~MsgListView();

    QHash<QString, MsgWidgetItem*> g_listFriendItem;                        // 键：好友用户ID，值：对应的listvViewItem
    QHash<QString, MsgWidgetItem*> g_listGroupItem;                         // 键：群组ID，值：对应的listViewItem
    void addUserItemSingle(const User& user);                               // 添加单个好友Item
    void addGroupItemSingle(const Group& group);                            // 添加单个群组Item
    QListView* getListView();

signals:
    void signalSwitchChat(int chatType, const QString& id);
    void signalChangeLable(User user);
    void signalAddRemark(const QString& userId, const QString& newName);
    void signalRenameGroup(const QString& groupId, const QString& newName);

public slots:
    void sltDeletedRelation(const QString& id, int role);                   // 被删除好友
    void sltRenamedGroup(const QString& groupId, const QString& newName);   // 群名被别人改

private slots:
    void sltItemClicked(const QModelIndex &index);                          // item被点击
    void showContextMenu(const QPoint& pos);                                // 展示菜单
    void sltAddFriendRemark(const QString& userId);                         // 添加好友备注
    void sltRenameGroup(const QString& id);                                 // 群名被自己改

private:
    Ui::MsgListView *ui;

    ClientSocket* m_client;
    QStandardItemModel* model;
    MsgWidgetItem* m_lastClicked;

    void addUserItemList();
    void addGroupItemList();
    void addFriendItem();
    QStandardItem* findItemById(const QString&id);                          // 通过ID遍历查找StandardItem
};

#endif // MsgListView_H
