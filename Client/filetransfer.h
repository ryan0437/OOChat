#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include "structures.h"

#include <QObject>
#include <QTcpSocket>
#include <QFile>

///////////////////////////////////////////////////////
/// \brief The FileTransfer class
/// 与服务器传输文件
class FileTransfer : public QObject
{
    Q_OBJECT
public:
    explicit FileTransfer(QObject *parent = nullptr);

    void sendFile(const ChatMessage& message);          // 发送文件
    void recvFile(const QString& fileUUID, int mode);   // 接收文件
    qint64 getTotalBytes();                             // 获取当前传输文件的总大小
    qint64 getBytesSent();                              // 获取已经发送的文件大小
    qint64 getBytesReceived();                          // 获取已经接收的文件大小

signals:
    void signalTransferFinished();                      // 文件传输完毕
    void signalDownloadFinished();                      // 文件下载完毕

public slots:
    void sltSendFile(const ChatMessage& message);           // 发送文件
    void sltRecvFile(const QString& fileUUID, int mode);    // 下载文件 mode 1:聊天文件 0:资源文件

private slots:
    void sltConnected();                                // 与文件服务器连接上
    void sltDisconnected();                             // 与文件服务器断开连接
    void sltReadyReadResponse();                        // 有可读的信息
    void sltReadyReadDownload();                        // 有可读的下载信息

private:
    QTcpSocket *m_serverSocket;

    QFile m_file;               // 当前传输的文件
    QString curFileName;        // 传输文件名
    qint64 totalBytes;          // 总字节数
    qint64 bytesSent;           // 已发送的字节数
    qint64 bytesReceived;       // 已接收的字节数
    QByteArray buffer;          // 缓冲区
    bool isTransferCompleted;   // 传输是否结束标识符
    int transferMode;           // 传输模式

    void sendFileByBlock();     // 分块发送文件
};

/////////////////////////////////////////////////////
/// \brief ResourceTransfer::ResourceTransfer
/// 封装多线程上传/下载 资源文件
class ResourceTransfer : public QObject
{
    Q_OBJECT
public:
    explicit ResourceTransfer(QObject *parent = nullptr);

    void uploadResource(const ChatMessage& message);            // 上传资源
    void downloadResource(const QString& avatarUUID);           // 下载资源

signals:
    void signalUploadFinished();                                // 资源上传结束
    void signalResourceDownloaded(const QString& avatarPath);   // 资源下载结束

private:
    QThread* m_workerThread;
    FileTransfer* m_fileTransfer;
};

#endif // FILETRANSFER_H
