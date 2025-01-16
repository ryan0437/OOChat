#ifndef CUSTOMMENU_H
#define CUSTOMMENU_H

#include <QMenu>
#include <QWidget>
#include <QStyleOptionMenuItem>
#include <QPainter>

/////////////////////////////////////////
/// \brief The CustomMenu class
/// 简单美化的菜单
class CustomMenu : public QMenu
{
    Q_OBJECT
public:
    explicit CustomMenu(QWidget *parent = nullptr) : QMenu(parent)
    {
        this->setWindowFlag(Qt::NoDropShadowWindowHint);
        //this->setAttribute(Qt::WA_TranslucentBackground);
        this->setStyleSheet(R"(
                            QMenu{
                                background-color: white;
                                border-radius:10px;
                                border:none;
                            }
                            QMenu::item{
                                background:transparent;
                                padding: 10px;
                                margin: 5px;
                                font-family: "Microsoft YaHei";
                                font-size: 20px;
                                border-radius:10px;
                            }
                            QMenu::item:selected{
                                background-color:rgb(244,244,244);
                            }
                            QMenu::item:hover{
                                background-color:rgb(242,242,242);
                            }
                            )");
    }
};


#endif // CUSTOMMENU_H
