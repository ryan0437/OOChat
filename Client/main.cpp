#include "mainwindow.h"
#include "loginwidget.h"
#include "clientsocket.h"
#include "structures.h"

#include <QApplication>
#include <QMetaType>
#include <QVBoxLayout>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<ChatMessage>();
    qRegisterMetaType<User>();
    qRegisterMetaType<Group>();
    LoginWidget loginWidget;
    loginWidget.show();

    return a.exec();
}
