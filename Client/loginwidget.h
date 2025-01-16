#ifndef WIDGET_H
#define WIDGET_H
#include "customitem/framelesswidget.h"

#include <QWidget>
#include <QEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QPainter>

class ClientSocket;
class RegisterWidget;
class Account;

QT_BEGIN_NAMESPACE
namespace Ui { class FramelessWidget; }
QT_END_NAMESPACE

/////////////////////////////////////////////
/// \brief The LoginWidget class
/// 登录界面
/// 点击下方的“注册账号”可创建账号
class LoginWidget : public FramelessWidget
{
    Q_OBJECT

public:
    LoginWidget(QWidget *parent = nullptr);
    ~LoginWidget();

signals:
    void signalLoginQuest();

    void signalSendUserId(QString UserId);

private slots:
    void on_toolBtnClose_clicked();             // 点击关闭窗口按钮
    void on_btnLogin_clicked();                 // 点击登录按钮
    void on_btnRegister_clicked();              // 点击注册按钮

    void sltLoginSuccess();                     // 登录成功
    void sltLoginFail();                        // 登录失败
    void sltRegister(const Account& account);   // 发送注册信息
    void sltOnTextChanged(const QString& text); // 账号密码LineEdit输入文本改变

private:
    Ui::FramelessWidget *ui;
    QLabel *label;

    ClientSocket *client;
    RegisterWidget *registerWidget;

    void setConfrimBtnFalseStyle();             // 设置“登录”按钮可点击时的样式
    void setConfirmBtnTrueStyle();              // 设置“登录”按钮不可点击时的样式

protected:
    void paintEvent(QPaintEvent *event) override;
};

/////////////////////////////////////////////////
/// \brief The DragWidgetFilter class
/// 可拖动窗口，事件过滤
class DragWidgetFilter : public QObject
{
    Q_OBJECT
public:
    DragWidgetFilter(QObject *parent)
        :QObject(parent){}

    bool eventFilter(QObject* object, QEvent* event)
    {
        auto w = dynamic_cast<QWidget*>(object);
        if (!w) return false;

        if (event->type() == QEvent::MouseButtonPress)
        {
            auto ev = dynamic_cast<QMouseEvent*>(event);
            if (!ev) return false;

            pos = ev->pos();
        }
        else if (event->type() == QEvent::MouseMove)
        {
            auto ev = dynamic_cast<QMouseEvent*>(event);
            if (!ev) return false;

            if (ev->buttons() & Qt::LeftButton)
            {
                w->move(ev->globalPos() - pos);
            }

        }
        return QObject::eventFilter(object, event);
    }

private:
    QPoint pos;
};

#endif // WIDGET_H
