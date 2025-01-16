#ifndef ADDFRIENDGROUP_H
#define ADDFRIENDGROUP_H
#include "customitem/framelesswidget.h"
#include "structures.h"

#include <QWidget>
#include <QPainter>
#include <QPen>
#include <QEvent>

class QHBoxLayout;
class QVBoxLayout;
class QTextEdit;
class QSpacerItem;
class roundedLabel;
class QLabel;
class QPushButton;
class QLineEdit;

namespace Ui {
class AddFriendGroup;
}

///////////////////////////////////////////////////////
/// \brief The AddFriendGroup class
/// 添加好友/加入群组
class AddFriendGroup : public FramelessWidget
{
    Q_OBJECT

public:
    explicit AddFriendGroup(QWidget *parent = nullptr);
    ~AddFriendGroup();

signals:
    void signalSendFriendReq(const User& user, const QString& msg, const QDate& date);
    void signalSendMsg(int chatType, const QString& userId);
    void signalJoinGroup(const QVariant& data);

private slots:
    void on_btnClose_clicked();
    void on_btnMini_clicked();
    void on_btnAll_clicked();
    void on_btnUser_clicked();
    void on_btnGroup_clicked();
    void on_btnAddUser_clicked();
    void on_btnAddGroup_clicked();
    void on_searchIcon_clicked();

    void sltShowQueryResultUser(const QList<User> userList);
    void sltShowQueryResultGroup(const QList<Group> groupList);

private:
    Ui::AddFriendGroup *ui;

    QWidget *widgetMain;
    QHBoxLayout *hLayoutMain;
    roundedLabel *labelAvatar;
    QWidget* widgetInfo;
    QVBoxLayout *vLayoutInfo;
    QLabel* labelName;
    QLabel* labelId;
    QSpacerItem *spacerItem;
    QPushButton *btnAdd;

    void createItem(const QString& id, const QString& name, const QString& avatarUUID, int role);   // role: 1 User， 2 Group

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 背景白色
        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(this->rect().adjusted(0.5, 0.5, -0.5, -0.5), 20, 20); // 圆角矩形，半径 20px

        // 设置边框颜色
        QPen pen(Qt::gray);
        pen.setWidth(1.0);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(this->rect().adjusted(0.5, 0.5, -0.5, -0.5), 20, 20);
    }
    bool eventFilter(QObject* obj, QEvent* event) override;
};

////////////////////////////////////////////////////
/// \brief The CreateGroup class
/// 创建群组
class CreateGroup : public QWidget
{
    Q_OBJECT

public:
    explicit CreateGroup(QWidget *parent = nullptr);

signals:
    void signalCreateGroup(const QVariant& data);

private slots:
    void on_btnConfirm_clicked();
    void on_btnChooseAvatar_clicked();

private:
    QVBoxLayout *vLayoutMain;

    QHBoxLayout *hLayoutId;
    QLabel *labelId;
    QLineEdit *lineEditId;

    QHBoxLayout *hLayoutName;
    QLabel *labelName;
    QLineEdit *lineEditName;

    QHBoxLayout *hLayoutChooseAvatar;
    QLabel *labelAvatarPath;
    QLineEdit *lineEditAvatarPath;
    QPushButton *btnChooseAvatar;

    roundedLabel *labelAvatar;
    QPushButton *btnConfirm;

};

#endif // ADDFRIENDGROUP_H
