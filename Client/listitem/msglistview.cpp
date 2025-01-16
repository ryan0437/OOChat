#include "msglistview.h"
#include "ui_msglistview.h"
#include "msgwidgetitem.h"
#include "localdbmanager.h"
#include "customitem/custommenu.h"

#include <QStandardItem>
#include <QListView>
#include <QDebug>
#include <QMenu>
#include <QListView>
#include <QLineEdit>
#include <QPushButton>

MsgListView::MsgListView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MsgListView)
{
    ui->setupUi(this);
    m_client = ClientSocket::instance();
    m_lastClicked = nullptr;

    // 使用QListView和QStandardItemModel
    model = new QStandardItemModel(this);
    ui->listViewMsg->setModel(model);  // 绑定模型到QListView

    addUserItemList();
    addGroupItemList();
    connect(ui->listViewMsg, &QListView::clicked, this, &MsgListView::sltItemClicked);

    // 设置上下文菜单策略
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QListView::customContextMenuRequested, this, &MsgListView::showContextMenu);
    connect(ClientSocket::instance(), &ClientSocket::signalDeletedRelation, this, &MsgListView::sltDeletedRelation);
}

MsgListView::~MsgListView()
{
    delete ui;
}

void MsgListView::addUserItemList()
{
    // 添加多个MsgWidgetItem到模型中
    QHash<QString, User> g_friendList = ClientSocket::instance()->g_friendList;
    for (QHash<QString, User>::iterator it = g_friendList.begin(); it != g_friendList.end(); ++it)
    {
        User user = it.value();
        addUserItemSingle(user);
    }

    // 去掉水平滑动条
    ui->listViewMsg->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void MsgListView::addGroupItemList()
{
    QHash<QString, Group> groupList = m_client->g_groupList;
    for(QHash<QString, Group>::iterator group = groupList.begin(); group != groupList.end(); ++group)
    {
        addGroupItemSingle(*group);
    }
}

QStandardItem* MsgListView::findItemById(const QString&id)
{
    QStandardItem* item = nullptr;

    for(int row = 0; row < model->rowCount(); row++) {
        item = model->item(row);
        if (item->data(Qt::UserRole).toString() == id){
            break;
        }
    }
    return item;
}

void MsgListView::addUserItemSingle(const User& user)
{
    // 将MsgWidgetItem封装到QStandardItem中
    MsgWidgetItem* newMsg = new MsgWidgetItem(ChatTypes::USER, this);

    // 设置最近的聊天信息
    ChatMessage latestMsg = LocalDbManager::instance().getLastestMsg(user.id);
    qDebug() << "latest msg from widget " << user.id << ":" << latestMsg.text;
    newMsg->setLatestMsg(latestMsg.text);
    newMsg->setTime(latestMsg.time);

    // 添加StandardItem
    QStandardItem* item = new QStandardItem();
    item->setSizeHint(QSize(350, 100));

    // 将用户的ID与对应的item绑定
    item->setData(user.id, Qt::UserRole);
    item->setData(ChatTypes::USER, Qt::UserRole + 1);
    g_listFriendItem[user.id] = newMsg;

    newMsg->setInitItem(user);

    // 将MsgWidgetItem设置为QStandardItem的内容
    model->appendRow(item);
    ui->listViewMsg->setIndexWidget(item->index(), newMsg); // 通过index设置自定义widget
}

void MsgListView::addGroupItemSingle(const Group &group)
{
    MsgWidgetItem* newMsg = new MsgWidgetItem(ChatTypes::GROUP, this);

    ChatMessage latestMsg = LocalDbManager::instance().getLastestMsg(group.id);
    newMsg->setLatestMsg(latestMsg.text);
    newMsg->setTime(latestMsg.time);

    QStandardItem* item = new QStandardItem();
    item->setSizeHint(QSize(350,100));

    item->setData(group.id, Qt::UserRole);
    item->setData(ChatTypes::GROUP, Qt::UserRole + 1);
    g_listGroupItem[group.id] = newMsg;

    newMsg->setInitItem(group);

    model->appendRow(item);
    ui->listViewMsg->setIndexWidget(item->index(), newMsg);
}

QListView *MsgListView::getListView()
{
    return ui->listViewMsg;
}

void MsgListView::sltDeletedRelation(const QString &id, int role)
{
    QStandardItem* deletedItem = findItemById(id);
    if (deletedItem)
    {
        if (m_lastClicked && (m_lastClicked == g_listFriendItem[id] || m_lastClicked == g_listGroupItem[id])) {
            m_lastClicked = nullptr;
        }
        model->removeRow(deletedItem->row());

        if (role == 0) {
            delete g_listFriendItem[id];
            g_listFriendItem.remove(id);
        } else {
            delete g_listGroupItem[id];
            g_listGroupItem.remove(id);
        }
        qDebug() << "Item deleted for id:" << id;
        ui->listViewMsg->update();
    }
    else
    {
        qDebug() << "Item failed to delete for id:" << id;
    }
}

void MsgListView::sltItemClicked(const QModelIndex &index)
{
    QString id = index.data(Qt::UserRole).toString();   // 获取与对应index绑定的item的用户ID
    int chatType = index.data(Qt::UserRole + 1).toInt();
    emit signalSwitchChat(chatType, id);

    MsgWidgetItem* curClicked = qobject_cast<MsgWidgetItem*>(ui->listViewMsg->indexWidget(index));
    if (!curClicked) return;

    // 如果之前点击的项存在且不是当前点击的项，设置为未选中状态
    if (m_lastClicked && m_lastClicked != curClicked)
    {
        m_lastClicked->setPreviousClickedStyle();
    }

    curClicked->setCurrentClickedStyle();

    m_lastClicked = curClicked;
}

void MsgListView::showContextMenu(const QPoint& pos) {
    // 创建菜单
    CustomMenu contextMenu(this);

    // 获取右键点击的位置
    QModelIndex index = ui->listViewMsg->indexAt(pos);
    if (!index.isValid()) {
        return;
    }
    QString id = index.data(Qt::UserRole).toString();  // 获取绑定的 userId
    int chatType = index.data(Qt::UserRole + 1).toInt();

    if (chatType == ChatTypes::USER){
        QAction* removeAction = contextMenu.addAction("删除好友");
        QAction* addFriendRemark = contextMenu.addAction("添加好友备注");

        connect(removeAction, &QAction::triggered, this, [this, id, index]() {
            QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->listViewMsg->model());
            if (model) {
                int row = index.row();
                model->removeRow(row);  // 删除对应行的数据
                ClientSocket::instance()->deleteFriendGroup(id, 0);
                qDebug() << "Remove friend: " << id;
                m_lastClicked = nullptr;
            }
        });
        connect(addFriendRemark, &QAction::triggered, this, [this, id](){
            sltAddFriendRemark(id);
        });

    } else if (chatType == ChatTypes::GROUP) {
        //QAction* removeAction = contextMenu.addAction("解散群组");
        QAction* renameGroupAction = contextMenu.addAction("重命名群组");
        QAction* quitGroupAction = contextMenu.addAction("退出群组");

        /*connect(removeAction, &QAction::triggered, this, [this, id, index]() {
            QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->listViewMsg->model());
            if (model) {
                model->removeRow(index.row());  // 删除对应行的数据
                ClientSocket::instance()->deleteFriendGroup(id, 0);
                qDebug() << "Remove friend: " << id;

                if (m_lastClicked && m_lastClicked == ui->listViewMsg->indexWidget(index)) {
                    m_lastClicked = nullptr;
                }
            }
        });*/
        connect(renameGroupAction, &QAction::triggered, this, [this, id](){
            sltRenameGroup(id);
        });
        connect(quitGroupAction, &QAction::triggered, this, [this, id, index](){
            QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->listViewMsg->model());
            if (model) {
                int row = index.row();
                model->removeRow(row);
                ClientSocket::instance()->deleteFriendGroup(id, 1);
                m_lastClicked = nullptr;
                qDebug() << "remove group: " << id;
            }
        });
    }
    // 显示菜单
    contextMenu.exec(mapToGlobal(pos));
}

void MsgListView::sltAddFriendRemark(const QString &userId)
{
    QWidget* widgetAddRemark = new QWidget();
    QLineEdit *lineEditAddRemark = new QLineEdit(widgetAddRemark);
    QPushButton *btnAddRemark = new QPushButton(widgetAddRemark);
    QHBoxLayout* hLayoutMain = new QHBoxLayout(widgetAddRemark);

    btnAddRemark->setText("确认添加备注");
    widgetAddRemark->setWindowTitle("为好友"+userId+"添加备注");
    hLayoutMain->addWidget(lineEditAddRemark);
    hLayoutMain->addWidget(btnAddRemark);

    widgetAddRemark->show();
    connect(btnAddRemark, &QPushButton::clicked, this, [=](){
        widgetAddRemark->close();
        QString remark = lineEditAddRemark->text();
        MsgWidgetItem* widgetItem = g_listFriendItem[userId];
        QString newName = QString(remark + " (%1)").arg(widgetItem->getCurName());
        widgetItem->setName(newName);
        emit signalAddRemark(userId, newName);
    });
}

void MsgListView::sltRenameGroup(const QString &id)
{
    QWidget* widgetRenameGroup = new QWidget();
    QLineEdit *lineEditRename = new QLineEdit(widgetRenameGroup);
    QPushButton *btnRenameGroup = new QPushButton(widgetRenameGroup);
    QHBoxLayout* hLayoutMain = new QHBoxLayout(widgetRenameGroup);

    btnRenameGroup->setText("确认重命名");
    widgetRenameGroup->setWindowTitle("群组重命名");
    hLayoutMain->addWidget(lineEditRename);
    hLayoutMain->addWidget(btnRenameGroup);

    widgetRenameGroup->show();
    connect(btnRenameGroup, &QPushButton::clicked, this, [=](){
        MsgWidgetItem* groupItem = g_listGroupItem[id];
        QString newName = lineEditRename->text();
        groupItem->setName(newName);
        ClientSocket::instance()->renameGroup(id, newName);
        emit signalRenameGroup(id, newName);
    });
}

void MsgListView::sltRenamedGroup(const QString &groupId, const QString &newName)
{
    MsgWidgetItem* groupItem = g_listGroupItem[groupId];
    groupItem->setName(newName);
}
