#ifndef REGISTERWIDGET_H
#define REGISTERWIDGET_H

#include <QWidget>
class Account;

namespace Ui {
class RegisterWidget;
}

////////////////////////////////////////////
/// \brief The RegisterWidget class
/// 注册界面窗口
class RegisterWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RegisterWidget(QWidget *parent = nullptr);
    ~RegisterWidget();

signals:
    void signalRegister(const Account& account);    // 将注册信息传递到mainwindow

private slots:
    void on_btnChooseAvatar_clicked();              // 点击选择头像按钮
    void on_btnRegister_clicked();                  // 点击注册按钮

private:
    Ui::RegisterWidget *ui;
};

#endif // REGISTERWIDGET_H
