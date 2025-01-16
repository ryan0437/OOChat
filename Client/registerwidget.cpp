#include "registerwidget.h"
#include "ui_registerwidget.h"
#include "structures.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QUuid>

RegisterWidget::RegisterWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RegisterWidget)
{
    ui->setupUi(this);
    this->setWindowTitle("账号注册");
}

RegisterWidget::~RegisterWidget()
{
    delete ui;
}

void RegisterWidget::on_btnChooseAvatar_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择头像", "", "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif)");
    ui->lineEditAvatarPath->setText(filePath);
    ui->labelAavtar->setPixmap(QPixmap(filePath));
}

void RegisterWidget::on_btnRegister_clicked()
{
    Account accountInfo;
    accountInfo.account = ui->lineEditAccount->text();
    accountInfo.pwd = ui->lineEditPwd->text();
    accountInfo.name = ui->lineEditName->text();
    accountInfo.avatarPath = ui->lineEditAvatarPath->text();
    accountInfo.avatar = QUuid::createUuid().toString(QUuid::WithoutBraces);

    if (accountInfo.account.isEmpty() || accountInfo.pwd.isEmpty() || accountInfo.name.isEmpty() || accountInfo.avatarPath.isEmpty())
    {
       QMessageBox::warning(this, "注册账号错误", "请填写完整的信息！");
       qDebug() << "register error...";
       return;
    }

    emit signalRegister(accountInfo);
}
