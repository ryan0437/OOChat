#include "addfriendgroup.h"
#include "ui_addfriendgroup.h"
#include "clientsocket.h"
#include "customitem/roundedlabel.h"
#include "filetransfer.h"
#include "localdbmanager.h"

#include <QAction>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QThread>
#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QUuid>

AddFriendGroup::AddFriendGroup(QWidget *parent) :
    FramelessWidget(parent),
    ui(new Ui::AddFriendGroup)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);    // 浮动窗口
    this->setAttribute(Qt::WA_OpaquePaintEvent, true);
    this->setAttribute(Qt::WA_NoSystemBackground, true);

    // 添加搜索图标
    QAction* lineEditSearchAction = new QAction(ui->lineEditSearch);
    lineEditSearchAction->setIcon(QIcon(":/imgs/search_24.svg"));
    ui->lineEditSearch->addAction(lineEditSearchAction, QLineEdit::LeadingPosition);
    ui->lineEditSearch->installEventFilter(this);

    ui->btnAll->clicked();
    connect(lineEditSearchAction, &QAction::triggered, this, &AddFriendGroup::on_searchIcon_clicked);
    connect(ClientSocket::instance(), &ClientSocket::signalGotUser, this, &AddFriendGroup::sltShowQueryResultUser);
    connect(ClientSocket::instance(), &ClientSocket::signalGotGroup, this, &AddFriendGroup::sltShowQueryResultGroup);
}

AddFriendGroup::~AddFriendGroup()
{
    delete ui;
}

void AddFriendGroup::on_btnClose_clicked()
{
    this->close();
}

void AddFriendGroup::on_btnMini_clicked()
{
    this->showMinimized();
}

void AddFriendGroup::sltShowQueryResultUser(QList<User> userList)
{
    /*if (userList.isEmpty()) {
        QMessageBox::information(this, "全网搜索", "未找到相关用户/群组");
        return;
    }*/

    for(const User user : userList) {
        if (user.id == ClientSocket::instance()->g_Id) continue;    // 排除自己
        qDebug() << user.id << " " << user.name << " " << user.avatarUUID;
        createItem(user.id, user.name, user.avatarUUID, ChatTypes::USER);
    }
}

void AddFriendGroup::sltShowQueryResultGroup(const QList<Group> groupList)
{
    /*if (groupList.isEmpty()) {
        QMessageBox::information(this, "全网搜索", "未找到相关用户/群组");
        return;
    }*/

    for(const Group group : groupList) {
        qDebug() << group.id << " " << group.name << " " << group.avatarUUID;
        createItem(group.id, group.name, group.avatarUUID, ChatTypes::GROUP);
    }
}

void AddFriendGroup::createItem(const QString& id, const QString& name, const QString& avatarUUID, int role)    // role: 1 User， 2 Group
{
    widgetMain = new QWidget(this);
    hLayoutMain = new QHBoxLayout(widgetMain);
    labelAvatar = new roundedLabel;

    widgetInfo = new QWidget(widgetMain);
    vLayoutInfo = new QVBoxLayout(widgetInfo);
    labelName = new QLabel(widgetInfo);
    labelId = new QLabel(widgetInfo);
    vLayoutInfo->addWidget(labelName);
    vLayoutInfo->addWidget(labelId);

    btnAdd = new QPushButton(widgetMain);
    btnAdd->setFixedSize(QSize(90,35));
    btnAdd->setProperty("userId", id);
    btnAdd->setProperty("userName", name);
    btnAdd->setProperty("avatarUUID", avatarUUID);
    QFont font("Microsoft YaHei", 9);
    btnAdd->setFont(font);
    btnAdd->setStyleSheet("QPushButton{"
                          "background-color: white;"
                          "border-radius: 5px;"
                          "border: 1px solid rgb(214,214,214);"
                          "}"
                          "QPushButton:hover{"
                          "background-color:rgb(235,235,235);"
                          "}");

    hLayoutMain->addWidget(labelAvatar);
    hLayoutMain->addWidget(widgetInfo);
    hLayoutMain->addWidget(btnAdd);

    labelAvatar->setFixedSize(QSize(50, 50));

    QString avatarPath = "./chatpics/" + avatarUUID;
    if (!QFile::exists(avatarPath))
    {
        ResourceTransfer* resourceTransfer = new ResourceTransfer(this);
        resourceTransfer->downloadResource(avatarUUID);
        connect(resourceTransfer, &ResourceTransfer::signalResourceDownloaded, this, [=](const QString& avatarPath){
            labelAvatar->setPixmap(avatarPath);
        });
    } else
        labelAvatar->setPixmap(avatarPath);

    labelName->setText(name);
    labelId->setText(id);
    if (ClientSocket::instance()->g_friendList.contains(id) || ClientSocket::instance()->g_groupList.contains(id)) {
        btnAdd->setText("发消息");
        connect(btnAdd, &QPushButton::clicked, this, [=](){
            if (role == ChatTypes::USER)
                emit signalSendMsg(ChatTypes::USER, id);
            else
                emit signalSendMsg(ChatTypes::GROUP, id);
            this->close();
        });
    } else {
        if (role == ChatTypes::USER) {
            connect(btnAdd, &QPushButton::clicked, this, &AddFriendGroup::on_btnAddUser_clicked);
            btnAdd->setText("添加好友");
        } else {
            btnAdd->setText("加入群组");
            connect(btnAdd, &QPushButton::clicked, this, [=](){
                Group group(id, name, avatarUUID);
                QVariant groupVariant = QVariant::fromValue(group);
                emit signalJoinGroup(groupVariant);
            });
        }
    }

    QListWidget* curListWidget = ui->stackedWidget->currentWidget()->findChild<QListWidget*>();
    QListWidgetItem *item = new QListWidgetItem(curListWidget);
    item->setData(Qt::UserRole, id);
    item->setSizeHint(QSize(curListWidget->width(), 100));
    curListWidget->setItemWidget(item, widgetMain);
}

bool AddFriendGroup::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->lineEditSearch && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        {
            on_searchIcon_clicked();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AddFriendGroup::on_btnAll_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageAll);
}

void AddFriendGroup::on_btnUser_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageUser);
}

void AddFriendGroup::on_btnGroup_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->pageGroup);
}

void AddFriendGroup::on_btnAddGroup_clicked()
{

}

void AddFriendGroup::on_btnAddUser_clicked()
{
    QPushButton* clickedBtn = qobject_cast<QPushButton*>(sender());

    // 添加好友申请备注
    QWidget *widgetSetMessage = new QWidget();
    QHBoxLayout *hLayoutMessage = new QHBoxLayout(widgetSetMessage);
    QLineEdit *lineEditMessage = new QLineEdit(widgetSetMessage);
    QPushButton *btnConfirm = new QPushButton(widgetSetMessage);
    hLayoutMessage->addWidget(lineEditMessage);
    hLayoutMessage->addWidget(btnConfirm);

    btnConfirm->setFixedSize(QSize(80, 30));
    btnConfirm->setText("确认");
    widgetSetMessage->setWindowTitle("为好友申请添加备注");
    widgetSetMessage->setFixedWidth(500);
    widgetSetMessage->show();

    connect(btnConfirm, &QPushButton::clicked, this, [=](){
        if (clickedBtn) {
            QString message = lineEditMessage->text();
            if (message.isEmpty()) message = "无";
            QString userId = clickedBtn->property("userId").toString();
            QString userName = clickedBtn->property("userName").toString();
            QString avatarUUID = clickedBtn->property("avatarUUID").toString();
            User user(userId, userName, static_cast<Status>(0), avatarUUID);
            ClientSocket::instance()->sendAddFriendRequest(userId, message);
            emit signalSendFriendReq(user, message, QDate::currentDate());
            widgetSetMessage->close();
            QMessageBox::information(this, "添加好友", "成功向" + userId + "发出好友申请！");
        }
    });
}

void AddFriendGroup::on_searchIcon_clicked()
{
    QWidget* curPage = ui->stackedWidget->currentWidget();
    QString searchedId = ui->lineEditSearch->text();
    if (searchedId.isEmpty()) {
        QMessageBox::warning(this, "全网搜索", "当前输入关键字为空！");
        return;
    }
    ui->stackedWidget->currentWidget()->findChild<QListWidget*>()->clear();

    // mode 1模糊搜索 0精确搜索，按照userId or groupId
    if (curPage == ui->pageAll){
        ClientSocket::instance()->getUserProfile(searchedId, 1);
        ClientSocket::instance()->getGroupProfile(searchedId, 1);
    } else if (curPage == ui->pageUser) {
        ClientSocket::instance()->getUserProfile(searchedId, 1);
    } else {
        ClientSocket::instance()->getGroupProfile(searchedId, 1);
    }
}

///////////////////////////////////////////////////////////////
/// \brief CreateGroup::CreateGroup 创建群组界面
/// \param parent
///
CreateGroup::CreateGroup(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    vLayoutMain = new QVBoxLayout(this);
    hLayoutId = new QHBoxLayout;
    labelId = new QLabel("群组ID：", this);
    lineEditId = new QLineEdit(this);
    hLayoutId->addWidget(labelId);
    hLayoutId->addWidget(lineEditId);

    hLayoutName = new QHBoxLayout;
    labelName = new QLabel("群组名称：", this);
    lineEditName = new QLineEdit(this);
    hLayoutName->addWidget(labelName);
    hLayoutName->addWidget(lineEditName);

    hLayoutChooseAvatar = new QHBoxLayout;
    labelAvatarPath = new QLabel("头像路径：");
    lineEditAvatarPath = new QLineEdit(this);
    btnChooseAvatar = new QPushButton("选择头像", this);
    hLayoutChooseAvatar->addWidget(labelAvatarPath);
    hLayoutChooseAvatar->addWidget(lineEditAvatarPath);
    hLayoutChooseAvatar->addWidget(btnChooseAvatar);

    labelAvatar = new roundedLabel(this);
    btnConfirm = new QPushButton("确认创建群组", this);
    vLayoutMain->addLayout(hLayoutId);
    vLayoutMain->addLayout(hLayoutName);
    vLayoutMain->addLayout(hLayoutChooseAvatar);
    vLayoutMain->addWidget(labelAvatar);
    vLayoutMain->addWidget(btnConfirm);

    labelAvatar->setFixedSize(100,100);

    connect(btnChooseAvatar, &QPushButton::clicked, this, &CreateGroup::on_btnChooseAvatar_clicked);
    connect(btnConfirm, &QPushButton::clicked, this, &CreateGroup::on_btnConfirm_clicked);
}

void CreateGroup::on_btnConfirm_clicked()
{
    QString id = lineEditId->text();
    QString name = lineEditName->text();
    QString avatarUUID = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString avatarPath = lineEditAvatarPath->text();
    if (id.isEmpty() || name.isEmpty() || avatarPath.isEmpty()) {
        QMessageBox::warning(this, "创建群组错误", "请填写完整的信息！");
        qDebug() << "create group error...";
        return;
    }

    Group group(id, name, avatarUUID);
    QFileInfo fileInfo(avatarPath);
    ChatMessage message;
    message.type = ChatMessage::File;
    message.sender = ClientSocket::instance()->g_Id;
    message.receiver = "群组头像";
    message.fileName = fileInfo.fileName();
    message.filePath = avatarPath;
    message.fileSize = fileInfo.size();
    message.fileUUID = avatarUUID;

    ResourceTransfer *resourceTransfer = new ResourceTransfer(this);
    resourceTransfer->uploadResource(message);
    connect(resourceTransfer, &ResourceTransfer::signalUploadFinished, this, [=](){
        Group group(id, name, avatarUUID);
        ClientSocket::instance()->createGroup(group);
        QMessageBox::information(this, "创建群组", "创建群组" + id + "成功！");

        QVariant groupVariant = QVariant::fromValue(group);
        emit signalCreateGroup(groupVariant);
        this->close();
    });
}

void CreateGroup::on_btnChooseAvatar_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择头像", "", "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)");
    lineEditAvatarPath->setText(filePath);
    labelAvatar->setPixmap(QPixmap(filePath));
}
