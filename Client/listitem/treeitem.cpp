#include "treeitem.h"
#include "customitem/roundedlabel.h"
#include "filetransfer.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFile>
#include <QDebug>

ItemSort::ItemSort(QWidget *parent) : QWidget(parent)
{
    this->setStyleSheet("background:transparent;border:none;");
    this->setFixedHeight(60);
    hLayoutSort = new QHBoxLayout(this);
    labelSortName = new QLabel(this);
    labelSortNum = new QLabel(this);
    font = QFont("Microsoft YaHei");

    hLayoutSort->setContentsMargins(10,0,20,0);
    hLayoutSort->setSpacing(0);
    QFont fontSortName = font;
    QFont fontSortNum = font;
    fontSortName.setPointSize(10);
    fontSortNum.setPointSize(9);
    labelSortName->setFont(fontSortName);
    labelSortNum->setFont(fontSortNum);

    labelSortNum->setStyleSheet("color:rgb(164,164,164);");
    labelSortNum->setFixedWidth(20);
    labelSortNum->setText("0");
    hLayoutSort->addWidget(labelSortName);
    hLayoutSort->addWidget(labelSortNum);
}

void ItemSort::setSortName(const QString &sortName)
{
    labelSortName->setText(sortName);
}

void ItemSort::addSortNum()
{
    int curNum = labelSortNum->text().toInt();
    labelSortNum->setText(QString::number(++curNum));
}

ItemUser::ItemUser(QWidget *parent) : QWidget(parent)
{
    font = QFont("Microsoft YaHei");
    hLayoutMain = new QHBoxLayout(this);
    // 头像
    widgetAvatar = new QWidget(this);
    labelAvatar = new roundedLabel(widgetAvatar); labelAvatar->setPixmap(QPixmap(":/imgs/head_icon7.jpg"));  //test
    hLayoutAvatar = new QHBoxLayout(widgetAvatar);

    widgetAvatar->setFixedWidth(70);
    labelAvatar->setFixedSize(QSize(60,60));
    hLayoutAvatar->setContentsMargins(0,0,0,0);
    hLayoutAvatar->setSpacing(0);
    hLayoutAvatar->addWidget(labelAvatar, Qt::AlignCenter);
    // 信息
    widgetInfo = new QWidget(this);
    labelName = new QLabel(widgetInfo);
    labelStatus = new QLabel(widgetInfo);
    vLayoutInfo = new QVBoxLayout(widgetInfo);

    QFont fontName = font;
    fontName.setPointSize(10);
    labelName->setFont(fontName);
    QFont fontStatus = font;
    fontStatus.setPointSize(9);
    labelStatus->setFont(fontStatus);
    labelStatus->setStyleSheet("color:rgb(158,158,158)");

    vLayoutInfo->setContentsMargins(0,0,0,0);
    vLayoutInfo->setSpacing(5);
    vLayoutInfo->addWidget(labelName);
    vLayoutInfo->addWidget(labelStatus);

    hLayoutMain->addWidget(widgetAvatar);
    hLayoutMain->addWidget(widgetInfo);

    this->setStyleSheet("QLabel{background:transparent;}"
                        "QWidget{background:transparent;}");
    this->setFixedHeight(110);
    labelStatus->setText("");
}

void ItemUser::setAvatar(const QString &avatarUUID)
{
    QString avatarPath = "./chatpics/" + avatarUUID;
    if (!QFile::exists(avatarPath)) {
        qDebug() << "用户头像不存在，需要下载！";
        ResourceTransfer* resourceTransfer = new ResourceTransfer(this);
        resourceTransfer->downloadResource(avatarUUID);
        connect(resourceTransfer, &ResourceTransfer::signalResourceDownloaded, this, [=](const QString& avatarPath){
            labelAvatar->setPixmap(avatarPath);
        });
    } else {
        labelAvatar->setPixmap(avatarPath);
    }
}

void ItemUser::setName(const QString &name)
{
    labelName->setText(name);
}
