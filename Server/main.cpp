#include "tcpserver.h"
#include "filetransfer.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //服务器端无界面
    Server server;
    FileServer fileServer;

    return a.exec();
}
