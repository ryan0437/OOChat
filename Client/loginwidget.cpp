#include "loginwidget.h"
#include "ui_loginwidget.h"
#include "mainwindow.h"
#include "registerwidget.h"
#include "clientsocket.h"
#include "structures.h"
#include "filetransfer.h"

#include <QMovie>
#include <QDebug>
#include <QMessageBox>
#include <QTcpSocket>
#include <QDebug>
#include <QThread>
#include <QFileInfo>

LoginWidget::LoginWidget(QWidget *parent)
    : FramelessWidget(parent)
    , ui(new Ui::FramelessWidget)
{
    ui->setupUi(this);
    client = ClientSocket::instance();
    this->setWindowTitle("OO");
    ui->labelIcon->setFixedWidth(120);
    ui->labelIcon->setFixedHeight(120);
    this->setFixedSize(473, 663);
    ui->lineEditPwd->setEchoMode(QLineEdit::Password);


    label = new QLabel(this);
    label->setStyleSheet("background-color: transparent;");
    label->setFixedSize(size());        // label填充整个窗口
    label->setScaledContents(true);     // label内容缩放
    label->lower();

    QMovie *movie = new QMovie(":/imgs/loginBackGround.gif");
    label->setMovie(movie);
    movie->start();

    connect(ui->radioButton, &QRadioButton::toggled, this, [=](bool checked){
        if (checked && !ui->lineEditAccount->text().isEmpty() && !ui->lineEditPwd->text().isEmpty())
            setConfirmBtnTrueStyle();
        else
            setConfrimBtnFalseStyle();
    });
    connect(ui->lineEditAccount, &QLineEdit::textChanged, this, &LoginWidget::sltOnTextChanged);
    connect(ui->lineEditPwd, &QLineEdit::textChanged, this, &LoginWidget::sltOnTextChanged);
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::on_toolBtnClose_clicked()
{
    this->close();
}

void LoginWidget::on_btnRegister_clicked()
{
    registerWidget = new RegisterWidget();
    registerWidget->show();

    connect(registerWidget, &RegisterWidget::signalRegister, this, &LoginWidget::sltRegister);
}

void LoginWidget::on_btnLogin_clicked()
{
    if (client->getStatus())
    {
        QString UserId = ui->lineEditAccount->text();
        QString userPwd = ui->lineEditPwd->text();
        client->sendLoginQuest(UserId, userPwd);

        connect(client, &ClientSocket::signalLoginSuccessed, this, &LoginWidget::sltLoginSuccess);
        connect(client, &ClientSocket::signalLoginFailed, this, &LoginWidget::sltLoginFail);
    }
}

void LoginWidget::sltLoginSuccess()
{
    disconnect(client, &ClientSocket::signalLoginSuccessed, this, &LoginWidget::sltLoginSuccess);
    // 将登录的用户ID赋值给登入成功的ClientSocket
    QString UserId = ui->lineEditAccount->text();
    ClientSocket* client = ClientSocket::instance();
    client->g_Id = UserId;

    MainWindow* mainwindow = new MainWindow;
    mainwindow->show();
    this->close();
}

void LoginWidget::sltLoginFail()
{
    qDebug() << "login failed...";
    QMessageBox::warning(this, "登录", "账号或者密码错误！");
}

void LoginWidget::sltRegister(const Account& account)
{
    client->sendRegisterInfo(account);

    QFileInfo fileInfo(account.avatarPath);
    ChatMessage message;
    message.type = ChatMessage::File;
    message.sender = account.account;
    message.receiver = "用户头像";
    message.fileName = fileInfo.fileName();
    message.filePath = account.avatarPath;
    message.fileSize = fileInfo.size();
    message.fileUUID = account.avatar;

    ResourceTransfer *resourceTransfer = new ResourceTransfer(this);
    resourceTransfer->uploadResource(message);
    connect(resourceTransfer, &ResourceTransfer::signalUploadFinished, this, [=](){
        QMessageBox::information(this, "账号注册", "账号注册成功！");
    });
}

void LoginWidget::sltOnTextChanged(const QString &text)
{
    Q_UNUSED(text);
    if (!ui->lineEditAccount->text().isEmpty() && !ui->lineEditPwd->text().isEmpty() && ui->radioButton->isChecked())
        setConfirmBtnTrueStyle();
    else
        setConfrimBtnFalseStyle();
}

void LoginWidget::setConfrimBtnFalseStyle()
{
    ui->btnLogin->setEnabled(false);
    ui->btnLogin->setStyleSheet("background-color: rgb(158, 218, 255);");
}

void LoginWidget::setConfirmBtnTrueStyle()
{
    ui->btnLogin->setEnabled(true);
    ui->btnLogin->setStyleSheet("background-color: rgb(19, 133, 255);");
}

void LoginWidget::paintEvent(QPaintEvent *){
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  // 抗锯齿
    QPainterPath path;
    path.addRoundedRect(rect(), 12, 12);
}
