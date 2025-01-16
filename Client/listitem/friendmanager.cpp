#include "friendmanager.h"
#include "ui_friendmanager.h"
#include "treeitem.h"
#include "clientsocket.h"
#include "structures.h"
#include "customitem/roundedlabel.h"
#include "filetransfer.h"
#include "localdbmanager.h"

#include <QPropertyAnimation>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QSpacerItem>

FriendManager::FriendManager(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FriendManager)
{
    ui->setupUi(this);

    lastClickedButton = ui->btnFriend;

    btnBackground = new QWidget(ui->widgetChooseBtn);
    qDebug() << ui->btnFriend->geometry();

    btnBackground->stackUnder(ui->btnFriend);
    btnBackground->setStyleSheet("background-color:white;"
                                 "border-radius:10px");
    ui->btnFriend->raise();
    ui->btnGroup->raise();

    initTree();
}

FriendManager::~FriendManager()
{
    delete ui;
}

void FriendManager::on_btnFriend_clicked()
{
    ui->stackedWidgetTree->setCurrentWidget(ui->pageFriend);
    ui->btnFriend->setStyleSheet("border:none;"
                                 "border-radius:10px;"
                                 "background-color:transparent;"
                                 "color:rgb(0,153,255);");
    ui->btnGroup->setStyleSheet("border:none;"
                                 "border-radius:10px;"
                                 "background-color:transparent;"
                                 "color:rgb(127,127,127);");
    animateBackground(ui->btnFriend->geometry());
    lastClickedButton = ui->btnFriend;
    qDebug() << ui->btnFriend->geometry();
}

void FriendManager::on_btnGroup_clicked()
{
    ui->stackedWidgetTree->setCurrentWidget(ui->pageGroup);
    ui->btnFriend->setStyleSheet("border:none;"
                                 "border-radius:10px;"
                                 "background-color:transparent;"
                                 "color:rgb(127,127,127);");
    ui->btnGroup->setStyleSheet("border:none;"
                                 "border-radius:10px;"
                                 "background-color:transparent;"
                                 "color:rgb(0,153,255);");
    animateBackground(ui->btnGroup->geometry());
    lastClickedButton = ui->btnGroup;
}

void FriendManager::sltItemFriendClicked(QTreeWidgetItem *item)
{
    if (item->childCount() > 0)
    {
        if (item->isExpanded())
            item->setExpanded(false);
        else
            item->setExpanded(true);
    } else {
        // 当点击的item没有子节点时说明这个item是用户，点击时就转到主界面的对话框
        QString id = item->data(0, Qt::UserRole).toString();    // 获取item绑定的ID
        emit signalSwitchToChat(ChatTypes::USER, id);
    }
}

void FriendManager::sltItemGroupClicked(QTreeWidgetItem *item)
{
    if (item->childCount() > 0)
    {
        if (item->isExpanded())
            item->setExpanded(false);
        else
            item->setExpanded(true);
    } else {
        // 当点击的item没有子节点时说明这个item是用户，点击时就转到主界面的对话框
        QString id = item->data(0, Qt::UserRole).toString();    // 获取item绑定的ID
        emit signalSwitchToChat(ChatTypes::GROUP, id);
    }
}

void FriendManager::addFriendItem()
{
    m_friendList = ClientSocket::instance()->g_friendList;
    QString sortName = "我的好友";  // 待添加好友分组功能
    QTreeWidgetItem* rootItem = m_rootItem[sortName];

    QWidget* widgetItem = ui->treeWidgetFriend->itemWidget(rootItem, 0);
    ItemSort* rootSort = qobject_cast<ItemSort*>(widgetItem);

    for(QHash<QString, User>::iterator it = m_friendList.begin(); it != m_friendList.end(); it++)
    {
        User user = it.value();
        ItemUser *itemUser = new ItemUser();
        itemUser->setName(user.name);
        itemUser->setAvatar(user.avatarUUID);

        QTreeWidgetItem *childItem = new QTreeWidgetItem(rootItem);
        ui->treeWidgetFriend->setItemWidget(childItem, 0, itemUser);
        childItem->setSizeHint(0, QSize(ui->treeWidgetFriend->width(), 110));

        childItem->setData(0, Qt::UserRole, user.id);
        rootSort->addSortNum();
    }
}

void FriendManager::addGroupItem()
{
    m_groupList = ClientSocket::instance()->g_groupList;
    QString sortName = "我加入的群聊";
    QTreeWidgetItem* rootItem = m_rootItem[sortName];

    QWidget* widgetItem = ui->treeWidgetGroup->itemWidget(rootItem, 0);
    ItemSort* rootSort = qobject_cast<ItemSort*>(widgetItem);

    for(QHash<QString, Group>::iterator it = m_groupList.begin(); it != m_groupList.end(); it++)
    {
        Group group = it.value();
        ItemUser *itemUser = new ItemUser();
        itemUser->setName(group.name);
        itemUser->setAvatar(group.avatarUUID);

        QTreeWidgetItem *childItem = new QTreeWidgetItem(rootItem);
        ui->treeWidgetGroup->setItemWidget(childItem, 0, itemUser);
        childItem->setSizeHint(0, QSize(ui->treeWidgetGroup->width(), 110));

        childItem->setData(0, Qt::UserRole, group.id);
        rootSort->addSortNum();
    }
}

void FriendManager::showEvent(QShowEvent *event)
{
    // 在窗口显示后设置btnBackground的位置，否则btnBackground调用setGrometry方法时btnFriend未渲染。
    btnBackground->setGeometry(ui->btnFriend->geometry());
    QWidget::showEvent(event);  // 确保调用父类的showEvent
}

void FriendManager::initTree()
{
    ui->treeWidgetFriend->setHeaderHidden(true);
    ui->treeWidgetGroup->setHeaderHidden(true);
    ui->treeWidgetFriend->setColumnCount(1);
    ui->treeWidgetGroup->setColumnCount(1);
    ui->treeWidgetFriend->setIndentation(0);
    ui->treeWidgetGroup->setIndentation(0);
    addSort(ui->treeWidgetFriend, "我的设备");
    addSort(ui->treeWidgetFriend, "机器人");
    addSort(ui->treeWidgetFriend, "特别关心");
    addSort(ui->treeWidgetFriend, "我的好友");
    addSort(ui->treeWidgetGroup, "置顶群聊");
    addSort(ui->treeWidgetGroup, "未命名的群聊");
    addSort(ui->treeWidgetGroup, "我创建的群聊");
    addSort(ui->treeWidgetGroup, "我管理的群聊");
    addSort(ui->treeWidgetGroup, "我加入的群聊");

    addFriendItem();
    addGroupItem();
    connect(ui->treeWidgetFriend, &QTreeWidget::itemClicked, this, &FriendManager::sltItemFriendClicked);
    connect(ui->treeWidgetGroup, &QTreeWidget::itemClicked, this, &FriendManager::sltItemGroupClicked);
}

void FriendManager::animateBackground(const QRect &targetGeometry)
{
    QPropertyAnimation *animation = new QPropertyAnimation(btnBackground, "geometry");
    animation->setDuration(500);  // 动画时长 500 毫秒
    animation->setStartValue(lastClickedButton->geometry());
    animation->setEndValue(targetGeometry);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void FriendManager::addSort(QTreeWidget* treeWidget, const QString& sortName)
{
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(treeWidget);
    ItemSort *newSort = new ItemSort();
    rootItem->setIcon(0, QIcon(":/imgs/arrow_right_small_16.svg"));
    newSort->setSortName(sortName);
    treeWidget->setItemWidget(rootItem, 0, newSort);
    //addSortItem(treeWidget, rootItem);
    m_rootItem[sortName] = rootItem;
}

void FriendManager::addSortItem(QTreeWidget* treeWidget, QTreeWidgetItem *rootItem)
{
    QTreeWidgetItem *childItem = new QTreeWidgetItem(rootItem);
    ItemUser *itemUser = new ItemUser();
    treeWidget->setItemWidget(childItem, 0, itemUser);
    childItem->setSizeHint(0, QSize(treeWidget->width(), 110));
}

void FriendManager::on_btnFriendMsg_clicked()
{
    emit signalSwitchToRequest();
}

void FriendManager::on_btnGroupMsg_clicked()
{

}

////////////////////////////////////////////////////////////////
/// \brief RequestList::RequestList 请求列表widget
/// \param parent
///
RequestList::RequestList(QWidget *parent) : QWidget(parent)
{
    vLayoutThis = new QVBoxLayout(this);
    widgetTop = new QWidget(this);
    hLayoutTop = new QHBoxLayout(widgetTop);
    labelRequest = new QLabel("好友通知", widgetTop);
    spacerTop = new QSpacerItem(0, 60, QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnFilter = new QPushButton(widgetTop);
    btnClear = new QPushButton(widgetTop);
    listWidgetRequest = new QListWidget(this);

    hLayoutTop->addWidget(labelRequest);
    hLayoutTop->addSpacerItem(spacerTop);
    hLayoutTop->addWidget(btnFilter);
    hLayoutTop->addWidget(btnClear);
    vLayoutThis->addWidget(widgetTop);
    vLayoutThis->addWidget(listWidgetRequest);

    QFont font("Microsoft YaHei", 12);
    labelRequest->setFont(font);

    widgetTop->setFixedHeight(60);
    btnFilter->setIconSize(QSize(30,30));
    btnClear->setIconSize(QSize(30,30));
    btnFilter->setIcon(QIcon(":/imgs/filter_16.svg"));
    btnClear->setIcon(QIcon(":/imgs/del_16.svg"));

    listWidgetRequest->setSpacing(20);
    vLayoutThis->setSpacing(0);
    vLayoutThis->setContentsMargins(0,0,0,0);
    hLayoutTop->setSpacing(10);
    hLayoutTop->setContentsMargins(30,0,20,0);

    this->setStyleSheet("QPushButton{"
                        "background:transparent;"
                        "}"
                        "QListWidget{"
                        "background:transparent;"
                        "border:none;"
                        "}"
                        "QListWidget::item{"
                        "background:transparent;"
                        "border:none;"
                        "}");
    listWidgetRequest->setStyleSheet("QScrollBar:vertical{"
                                     "background-color: transparent;"
                                     "width:10px;"
                                     "margin:0px;"
                                     "border-radius:5px;"
                                     "}"
                                     "QScrollBar::handle:vertical{"
                                     "background-color: rgb(215,215,215);"
                                     "border-radius:5px;"
                                     "}"
                                     "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical{"
                                     "height:0px;"
                                     "}"
                                     "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical{"
                                     "background: transparent;"
                                     "}");

    connect(btnClear, &QPushButton::clicked, this, &RequestList::on_btnClear_clicked);
}

RequestList::~RequestList()
{
}

void RequestList::addRequestItem(const User &user, const QDate &date, const QString& message, int status)
{
    QWidget *widgetWhole = new QWidget(this);
    QHBoxLayout *hLayoutWhole = new QHBoxLayout(widgetWhole);
    QWidget *widgetMain = new QWidget(widgetWhole);
    QHBoxLayout *hLayoutMain = new QHBoxLayout(widgetMain);
    roundedLabel *labelAvatar = new roundedLabel(widgetMain);
    QWidget *widgetinfo = new QWidget(widgetMain);
    QVBoxLayout *vLayoutInfo = new QVBoxLayout(widgetinfo);
    QLabel *labelReqInfo = new QLabel(widgetinfo);
    QLabel *labelMessage = new QLabel(widgetinfo);
    QSpacerItem *spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
    QLabel *labelConfirmStatus = new QLabel(widgetMain);
    labelConfirmStatus->setVisible(false);

    hLayoutWhole->addWidget(widgetMain);
    hLayoutMain->addWidget(labelAvatar);
    vLayoutInfo->addWidget(labelReqInfo);
    vLayoutInfo->addWidget(labelMessage);
    hLayoutMain->addWidget(widgetinfo);
    hLayoutMain->addSpacerItem(spacer);

    widgetWhole->setFixedHeight(100);
    labelAvatar->setFixedSize(QSize(55,55));
    hLayoutWhole->setContentsMargins(60, 0, 60, 0);
    hLayoutWhole->setSpacing(0);
    hLayoutMain->setContentsMargins(40 ,20, 40 ,20);
    hLayoutMain->setSpacing(5);
    vLayoutInfo->setContentsMargins(0, 0, 0, 0);
    vLayoutInfo->setSpacing(5);

    QString infoStr = QString("<span style='color:#449CFF; font-family:Microsoft YaHei; font-size:9pt;'>%1</span> "
                              "<span style='color:black; font-family:Microsoft YaHei; font-size:9pt;'>%2</span> "
                              "<span style='color:#A4AAB7; font-family:Microsoft YaHei; font-size:9pt;'>%3</span>")
                             .arg(user.name)
                             .arg("请求添加为好友")
                             .arg(date.toString("yyyy-MM-dd"));
    QString messageStr = QString("<span style='color:#A4AAB7; font-family:Microsoft YaHei; font-size:9pt;'>留言：%1</span>").arg(message);
    labelReqInfo->setText(infoStr);
    labelMessage->setText(messageStr);

    setAvatar(labelAvatar, user.avatarUUID);
    widgetMain->setStyleSheet("QWidget{"
                              "background-color:white;"
                              "border-radius:10px;"
                              "}");


    if (status == RequestStatus::WaitingForSelf) {
        QPushButton *btnConfirm = new QPushButton("同意", widgetMain);
        QPushButton *btnRefuse = new QPushButton("拒绝", widgetMain);
        hLayoutMain->addWidget(btnConfirm);
        hLayoutMain->addWidget(btnRefuse);
        QString btnStyleSheet = QString("QPushButton{"
                                      "background-color: white;"
                                      "border-radius: 5px;"
                                      "border: 1px solid rgb(214,214,214);"
                                      "}"
                                      "QPushButton:hover{"
                                      "background-color:rgb(235,235,235);"
                                      "}");
        btnConfirm->setStyleSheet(btnStyleSheet);
        btnRefuse->setStyleSheet(btnStyleSheet);
        btnConfirm->setProperty("userId", user.id);
        btnRefuse->setProperty("userId", user.id);
        btnConfirm->setFixedSize(QSize(70,30));
        btnRefuse->setFixedSize(QSize(70,30));

        connect(btnConfirm, &QPushButton::clicked, this, [=](){
           btnConfirm->deleteLater();
           btnRefuse->deleteLater();
           QString statusStr = QString("<span style='color:#A4AAB7; font-family:Microsoft YaHei; font-size:9pt;'>%1</span>").arg("已同意");
           labelConfirmStatus->setText(statusStr);
           labelConfirmStatus->setVisible(true);
           hLayoutMain->addWidget(labelConfirmStatus);
           LocalDbManager::instance().updateRequestStatus(ClientSocket::instance()->g_Id, user.id, RequestStatus::Accepted);

           // 同意之后需要获取最新的对方信息和在线状态，再添加到好友列表
           ClientSocket::instance()->getUserProfile(user.id, [this](const User& user){
               QVariant userVariant = QVariant::fromValue(user);
               ClientSocket::instance()->sendConfirmFriendRequest(user, RequestStatus::Accepted);
               emit signalAcceptRequest(userVariant);
           });
        });
        connect(btnRefuse, &QPushButton::clicked, this, [=](){
            btnConfirm->deleteLater();
            btnRefuse->deleteLater();
            QString statusStr = QString("<span style='color:#A4AAB7; font-family:Microsoft YaHei; font-size:9pt;'>%1</span>").arg("已拒绝");
            labelConfirmStatus->setVisible(true);
            labelConfirmStatus->setText(statusStr);
            hLayoutMain->addWidget(labelConfirmStatus);
            ClientSocket::instance()->sendConfirmFriendRequest(user.id, RequestStatus::Rejected);
            LocalDbManager::instance().updateRequestStatus(ClientSocket::instance()->g_Id, user.id, RequestStatus::Rejected);
        });
    } else {
        if (status == RequestStatus::WaitingForReceiver)
            labelConfirmStatus->setText("待确认");
        else if (status == RequestStatus::Accepted)
            labelConfirmStatus->setText("已同意");
        else if (status == RequestStatus::Rejected)
            labelConfirmStatus->setText("已拒绝");
        else if (status == RequestStatus::BeenAccepted)
            labelConfirmStatus->setText("已被接收");
        else if (status == RequestStatus::BeenRejected)
            labelConfirmStatus->setText("已被拒绝");
        labelConfirmStatus->setVisible(true);
        hLayoutMain->addWidget(labelConfirmStatus);
        QFont font("Microsoft YaHei", 9);
        labelConfirmStatus->setFont(font);
    }

    QListWidgetItem *item = new QListWidgetItem(listWidgetRequest);
    item->setSizeHint(widgetWhole->sizeHint());
    item->setFlags(Qt::NoItemFlags);          // 禁止item交互
    listWidgetRequest->insertItem(0, item);   // 从顶部插入Item
    listWidgetRequest->setItemWidget(item, widgetWhole);
}

void RequestList::on_btnClear_clicked()
{
    listWidgetRequest->clear();
}

void RequestList::setAvatar(roundedLabel* labelAvatar, const QString &avatarUUID)
{
    QString avatarPath = "./chatpics/" + avatarUUID;
    if (!QFile::exists(avatarPath)) {
        qDebug() << "用户头像不存在，需要下载！";
        ResourceTransfer *resourceTransfer = new ResourceTransfer(this);
        resourceTransfer->downloadResource(avatarUUID);
        connect(resourceTransfer, &ResourceTransfer::signalResourceDownloaded, this, [=](){
            labelAvatar->setPixmap(avatarPath);
        });
    } else {
        labelAvatar->setPixmap(avatarPath);
    }
}

void RequestList::showHistoryRequests()
{
    QList<QVariantMap> requestList = LocalDbManager::instance().getHistoryRequest();
    for(auto it = requestList.begin(); it != requestList.end(); ++it) {
        const QVariantMap &request = *it;
        QString peerId = request["peerId"].toString();
        QString peerName = request["peerName"].toString();
        QString avatarUUID = request["avatarUUID"].toString();
        QString message = request["message"].toString();
        User user(peerId, peerName, static_cast<Status>(0), avatarUUID);
        int status = request["status"].toInt();
        QDate date = QDate::fromString(request["date"].toString(), "yyyy-MM-dd");
        addRequestItem(user, date, message, status);
    }
}
