#ifndef FILESERVER_H
#define FILESERVER_H
#include "structures.h"

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>

/////////////////////////////////////////////////////
/// \brief The FileServer class
/// 多线程传输文件
class FileServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit FileServer(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override; // 重写 为新连接的下载请求创建子线程

};

////////////////////////////////////////////
/// \brief The FileTransferThread class
/// 传输文件线程
class FileTransferThread : public QObject
{
    Q_OBJECT

public:
    FileTransferThread(QObject *parent = nullptr);
    void setClientSocket(QTcpSocket* clientSocket);

signals:
    void signalTransferFinished();      // 文件传输结束

private slots:
    void sltReadyReadQuest();   // 读取上传/下载请求事务
    void sltDisconnected();     // 与客户端断开连接
    void sltReadyReadUpload();  // 接收上传文件的内容

private:
    QTcpSocket* m_clientSocket; // 客户端socket

    QFile m_file;               // 传输的文件
    QString header;             // 传输事务请求头
    QString curFileName;        // 当前传输的文件名
    qint64 totalBytes;          // 文件大小总字节数
    qint64 bytesSent;           // 已发送字节数
    qint64 bytesReceived;       // 已接收字节数
    QByteArray buffer;          // 缓冲区
    bool isTransferCompleted;   // 文件传输是否结束

    ChatMessage message;        // 储存当前传输文件消息内容

    QString generateUUID();     // 产生一个UUID
    void sendFileByBlock();     // 分块发送文件

};

#endif // FILESERVER_H
