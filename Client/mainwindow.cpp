#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "loginwidget.h"
#include "clientsocket.h"
#include "localdbmanager.h"
#include "listitem/chatwidget.h"
#include "customitem/custommenu.h"
#include "customitem/custombadge.h"
#include "menuaction/addfriendgroup.h"
#include "filetransfer.h"

#include <QDateTime>
#include <QAction>
#include <QMessageBox>
#include <QDesktopWidget>

#include <listitem/msgwidgetitem.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_isResizing = false;
    m_resizeEdge = 0;
    initUI();

    LocalDbManager::instance();
    ClientSocket* client = ClientSocket::instance();
    m_selfId = client->g_Id;
    client->getUserProfile(client->g_Id, [this](const User& user)
    {
        ClientSocket::instance()->g_Profile = user;
        QString avatarPath = "./chatpics/" + user.avatarUUID;
        if (!QFile::exists(avatarPath)){
            qDebug() << "用户头像不存在，需要下载！";
            ResourceTransfer* resourceTransfer = new ResourceTransfer(this);
            resourceTransfer->downloadResource(user.avatarUUID);
            connect(resourceTransfer, &ResourceTransfer::signalResourceDownloaded, this, [=](const QString& avatarPath){
                ui->labelUserAvatar->setPixmap(avatarPath);
            });
        } else {
            ui->labelUserAvatar->setPixmap(avatarPath);
        }
    });

    isGotFriendList = false;
    isGotGroupList = false;
    client->getFriendList();
    client->getGroupList();
    connect(client, &ClientSocket::signalGotFriendList, this, &MainWindow::sltGotFriendList);
    connect(client, &ClientSocket::signalGotGroupList, this, &MainWindow::sltGotGroupList);
    connect(client, &ClientSocket::signalRecvMsg, this, &MainWindow::sltSaveMsg);
    connect(client, &ClientSocket::signalGotAddFriendReq, this, &MainWindow::sltAddFriendRequest);
    connect(client, &ClientSocket::signalDeleteChatWidget, this, &MainWindow::sltDeleteFriendGroup);
    connect(client, &ClientSocket::signalDeletedRelation, this, &MainWindow::sltDeletedRelation);
    connect(client, &ClientSocket::signalRenamedGroup, this, &MainWindow::sltRenameGroup);
    connect(client, &ClientSocket::signalAddedFriend, this, &MainWindow::sltShowNewFriendGroup);

    connect(m_requestList, &RequestList::signalAcceptRequest, this, &MainWindow::sltShowNewFriendGroup);
}

MainWindow::~MainWindow()
{
    qDebug() << "Chat exit";
    delete ui;
    delete m_msgListWidget;
    delete ClientSocket::instance();
}

//切换到消息界面
void MainWindow::switchToMsgList()
{

    unreadBadge->move(ui->toolButtonMsg->x() + ui->toolButtonMsg->width() * 0.6,
                      ui->toolButtonMsg->y() + ui->toolButtonMsg->height() * 0.1);

    m_msgListWidget = new MsgListView(this);
    ui->stackedWidget->addWidget(m_msgListWidget);
    ui->stackedWidget->setCurrentWidget(m_msgListWidget);

    connect(m_msgListWidget, &MsgListView::signalSwitchChat, this, &MainWindow::sltSwitchChat);
    connect(m_msgListWidget, &MsgListView::signalAddRemark, this, &MainWindow::sltUpdateRemark);
    connect(m_msgListWidget, &MsgListView::signalRenameGroup, this, &MainWindow::sltRenameGroup);

    connect(ClientSocket::instance(), &ClientSocket::signalRenamedGroup, m_msgListWidget, &MsgListView::sltRenamedGroup);
}

void MainWindow::on_btnClose_clicked()
{
    this->close();
}

void MainWindow::on_btnMini_clicked()
{
    this->showMinimized();
}

void MainWindow::sltSwitchChat(int chatType, const QString& id)
{
    if (m_chatWidgetMap.contains(id))
    {
        ui->stackedWidgetChat->setCurrentWidget(m_chatWidgetMap[id]);

        int unreadCount = m_unreadMessageCount[id];
        if (unreadCount != 0) {
            unreadBadge->minusRead(m_unreadMessageCount[id]);
            if (chatType == ChatTypes::USER)
                m_msgListWidget->g_listFriendItem[id]->getWidgetBadge()->minusRead(m_unreadMessageCount[id]);
            else
                m_msgListWidget->g_listGroupItem[id]->getWidgetBadge()->minusRead(m_unreadMessageCount[id]);

            m_unreadMessageCount[id] = 0;
        }
    }
    else
    {
        ChatWidget *newChat;
        if (chatType == ChatTypes::USER)
            newChat = new ChatWidget(ClientSocket::instance()->g_friendList[id], m_msgListWidget->g_listFriendItem[id], this);
        else
            newChat = new ChatWidget(ClientSocket::instance()->g_groupList[id], m_msgListWidget->g_listGroupItem[id], this);

        ui->stackedWidgetChat->addWidget(newChat);
        ui->stackedWidgetChat->setCurrentWidget(newChat);
        m_chatWidgetMap[id] = newChat;

        int unreadCount = m_unreadMessageCount[id];
        if (unreadCount != 0) {
            unreadBadge->minusRead(m_unreadMessageCount[id]);
            if (chatType == ChatTypes::USER)
                m_msgListWidget->g_listFriendItem[id]->getWidgetBadge()->minusRead(m_unreadMessageCount[id]);
            else
                m_msgListWidget->g_listGroupItem[id]->getWidgetBadge()->minusRead(m_unreadMessageCount[id]);

            m_unreadMessageCount[id] = 0;
        }
    }
}

void MainWindow::sltSaveMsg(ChatMessage& message)
{
    QString sender = message.chatType == ChatTypes::USER ? message.sender : message.receiver;
    if (!m_chatWidgetMap.contains(sender))
    {
        ChatWidget *newChat;
        if (ClientSocket::instance()->g_friendList.contains(sender))
            newChat = new ChatWidget(ClientSocket::instance()->g_friendList[sender], m_msgListWidget->g_listFriendItem[sender], this);
        else
            newChat = new ChatWidget(ClientSocket::instance()->g_groupList[sender], m_msgListWidget->g_listGroupItem[sender], this);
        ui->stackedWidgetChat->addWidget(newChat);
        m_chatWidgetMap[sender] = newChat;
    }
    qDebug() << "收到消息：" << sender << message.text;

    // 更新未读消息数量
    if (ui->stackedWidgetChat->currentWidget() != m_chatWidgetMap[sender]) {
        unreadBadge->unreadIncrement();
        m_unreadMessageCount[sender]++;
        if (message.chatType == ChatTypes::USER)
            m_msgListWidget->g_listFriendItem[sender]->getWidgetBadge()->unreadIncrement();
        else if (message.chatType == ChatTypes::GROUP)
            m_msgListWidget->g_listGroupItem[sender]->getWidgetBadge()->unreadIncrement();
    }

    m_chatWidgetMap[sender]->addChatBubble(message);
    LocalDbManager::instance().saveMessage(message);
}

void MainWindow::sltGotFriendList()
{
    isGotFriendList = true;
    if (isGotGroupList && isGotFriendList)
        setFriendGroupList();
}

void MainWindow::sltGotGroupList()
{
    isGotGroupList = true;
    if (isGotGroupList && isGotFriendList)
        setFriendGroupList();
}

void MainWindow::sltAddFriendMenu()
{
    AddFriendGroup *addFriendGroup = new AddFriendGroup(this);
    addFriendGroup->move((m_desktop->width() - addFriendGroup->width()) / 2, (m_desktop->height() - addFriendGroup->height()) / 2);
    addFriendGroup->show();

    connect(addFriendGroup, &AddFriendGroup::signalSendFriendReq, this, &MainWindow::sltSendFriendRequest);
    connect(addFriendGroup, &AddFriendGroup::signalSendMsg, this, &MainWindow::sltSwitchChat);
    connect(addFriendGroup, &AddFriendGroup::signalJoinGroup, this, &MainWindow::sltShowNewFriendGroup);
}

void MainWindow::sltCreateGroup()
{
    CreateGroup *createGroup = new CreateGroup(this);
    createGroup->move((m_desktop->width() - createGroup->width()) / 2, (m_desktop->height() - createGroup->height()) / 2);
    createGroup->show();

    connect(createGroup, &CreateGroup::signalCreateGroup, this, &MainWindow::sltShowNewFriendGroup);
}

void MainWindow::sltAddFriendRequest(const User& user, const QString& msg, const QDate& date)
{

    m_requestList->addRequestItem(user, date, msg, RequestStatus::WaitingForSelf);
    LocalDbManager::instance().saveRequest(user, msg, date, WaitingForSelf);
    QMessageBox::information(this, "好友添加消息", user.id + "请求添加你为好友，请在好友管理查看！");
}

void MainWindow::sltSendFriendRequest(const User &user, const QString &msg, const QDate &date)
{
    m_requestList->addRequestItem(user, date, msg, RequestStatus::WaitingForReceiver);
    LocalDbManager::instance().saveRequest(user, msg, date, RequestStatus::WaitingForReceiver);
}

void MainWindow::sltDeleteFriendGroup(const QString &id)
{
    if (m_chatWidgetMap.contains(id)) {
        m_msgListWidget->getListView()->clearFocus();
        m_msgListWidget->getListView()->selectionModel()->clearSelection();
        ChatWidget* chatWidget = m_chatWidgetMap[id];
        ui->stackedWidgetChat->removeWidget(chatWidget);
        chatWidget->deleteLater();
    }
}

void MainWindow::sltDeletedRelation(const QString &id, int role)
{
    ui->stackedWidgetChat->setCurrentWidget(ui->pageBlank);
    if (m_chatWidgetMap.contains(id)) {
        m_msgListWidget->getListView()->clearFocus();
        m_msgListWidget->getListView()->selectionModel()->clearSelection();
        ChatWidget* chatWidget = m_chatWidgetMap[id];
        ui->stackedWidgetChat->removeWidget(chatWidget);

        if (role == 0)
            QMessageBox::information(this, "消息", "你被" + id + "删除了好友！");
        else
            QMessageBox::information(this, "消息", "你被" + id + "踢出了群聊！");
    }
}

void MainWindow::sltShowNewFriendGroup(const QVariant& info)
{
    ChatWidget* newChat;
    if (info.canConvert<User>()) {
        User user = info.value<User>();
        m_msgListWidget->addUserItemSingle(user);
        newChat = new ChatWidget(user, m_msgListWidget->g_listFriendItem[user.id], this);
        ui->stackedWidgetChat->addWidget(newChat);
        m_chatWidgetMap[user.id] = newChat;
        newChat->sendInitMessage("[{\"content\":\"我们已经成为好友，开始聊天吧\",\"type\":\"text\"}]");
    } else {
        Group group = info.value<Group>();
        ClientSocket::instance()->g_groupList[group.id] = group;
        ClientSocket::instance()->joinGroup(group.id);
        m_msgListWidget->addGroupItemSingle(group);
        newChat = new ChatWidget(group, m_msgListWidget->g_listGroupItem[group.id], this);
        ui->stackedWidgetChat->addWidget(newChat);
        m_chatWidgetMap[group.id] = newChat;
        newChat->sendInitMessage("[{\"content\":\"我是新来的群成员...\",\"type\":\"text\"}]");
    }
    //ui->stackedWidgetChat->setCurrentWidget(newChat);
}

void MainWindow::sltUpdateRemark(const QString &userId, const QString &newName)
{
    ClientSocket::instance()->addFriendRemark(userId, newName);
}

void MainWindow::sltRenameGroup(const QString &groupId, const QString &newName)
{
    if (m_chatWidgetMap.contains(groupId)) {
        ChatWidget* groupChatWidget = m_chatWidgetMap[groupId];
        groupChatWidget->setName(newName);
    }
}

void MainWindow::sltSwitchToPageBlank()
{
    ui->stackedWidgetChat->setCurrentWidget(ui->pageBlank);
}

void MainWindow::on_toolButtonFriend_clicked()
{
    ui->stackedWidget->setCurrentWidget(m_friendManager);
}

void MainWindow::on_toolButtonMsg_clicked()
{
    ui->stackedWidget->setCurrentWidget(m_msgListWidget);
}

void MainWindow::initUI()
{
    this->setWindowTitle("OO");
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_DeleteOnClose);           // 在窗口关闭后销毁
    this->resize(1500,1000);

    DragWidgetFilter *dragFilter = new DragWidgetFilter(this);
    this->installEventFilter(dragFilter);

    //使用QAction给搜索栏添加图标
    QAction* lineEditSearchAction = new QAction(ui->lineEditSearch);
    lineEditSearchAction->setIcon(QIcon(":/imgs/search_24.svg"));
    ui->lineEditSearch->addAction(lineEditSearchAction, QLineEdit::LeadingPosition);

    // 设置创建群组和添加好友/群组的菜单
    menuAddFriend = new CustomMenu(this);
    QAction *actionCreateGroup = new QAction("发起群聊", menuAddFriend);
    connect(actionCreateGroup, &QAction::triggered, this, &MainWindow::sltCreateGroup);
    QAction *actionAddFriend = new QAction("加好友/群", menuAddFriend);
    connect(actionAddFriend, &QAction::triggered, this, &MainWindow::sltAddFriendMenu);
    actionCreateGroup->setIcon(QIcon(":/imgs/add_circle.svg"));
    actionAddFriend->setIcon(QIcon(":/imgs/add_group_16.svg"));
    menuAddFriend->addAction(actionCreateGroup);
    menuAddFriend->addAction(actionAddFriend);
    ui->btnAdd->setMenu(menuAddFriend);
    ui->btnAdd->setPopupMode(QToolButton::InstantPopup);

    unreadBadge = new WidgetBadge(this);
    unreadBadge->setVisible(false);

    m_requestList = new RequestList(this);
    ui->stackedWidgetChat->addWidget(m_requestList);

    m_desktop = QApplication::desktop();
}

void MainWindow::setFriendGroupList()
{
    switchToMsgList();
    m_friendManager = new FriendManager(this);
    ui->stackedWidget->addWidget(m_friendManager);
    connect(m_friendManager, &FriendManager::signalSwitchToChat, this, &MainWindow::sltSwitchChat);
    connect(m_friendManager, &FriendManager::signalSwitchToRequest, this, [=](){
        ui->stackedWidgetChat->setCurrentWidget(m_requestList);
    });
    qDebug() << "got friend list...";
    m_requestList->showHistoryRequests();
}

void MainWindow::on_btnMax_clicked()
{
    if (isFullScreen())
        showNormal();
    else
        showFullScreen();
}
