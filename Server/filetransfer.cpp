#include "filetransfer.h"
#include "dbmanager.h"
#include "tcpserver.h"

#include <QDataStream>
#include <QDebug>
#include <QThread>
#include <QAbstractSocket>
#include <QDir>
#include <QUuid>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

FileServer::FileServer(QObject *parent) : QTcpServer(parent)
{
    bool ok = this->listen(QHostAddress::Any, 9998);
    if (!ok)
    {
        qDebug() << "listen failed";
        return;
    }
    qDebug() << "file server is listening port 9998...";
}

void FileServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *clientSocket = new QTcpSocket();
    if (clientSocket->setSocketDescriptor(socketDescriptor))
    {
        FileTransferThread *worker = new FileTransferThread();
        QThread *workerThread = new QThread();

        // clientSocket要和worker工作在同一个线程下
        worker->moveToThread(workerThread);
        clientSocket->moveToThread(workerThread);
        worker->setClientSocket(clientSocket);

        // 任务结束清理字线程
        connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
        connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

        workerThread->start();

    }else{
        delete  clientSocket;
    }
}

FileTransferThread::FileTransferThread(QObject *parent) :
    QObject(parent),
    curFileName(""),
    totalBytes(0),
    bytesReceived(0),
    buffer(0),
    isTransferCompleted(false)
{

}

void FileTransferThread::setClientSocket(QTcpSocket *clientSocket)
{
    m_clientSocket = clientSocket;
    if (m_clientSocket == nullptr)
    {
        qDebug() << "wrong...";
        return;
    }
    qDebug() << "new client socket connected...";

    //m_clientSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 1024 * 1024);
    //m_clientSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 1024 * 1024);

    connect(m_clientSocket, &QTcpSocket::readyRead, this, &FileTransferThread::sltReadyReadQuest);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &FileTransferThread::sltDisconnected);
}

void FileTransferThread::sltReadyReadQuest()
{
    // 断开分类请求的connect，将readyRread与上传文件槽函数连接
    disconnect(m_clientSocket, &QTcpSocket::readyRead, this, &FileTransferThread::sltReadyReadQuest);
    QDataStream in(m_clientSocket);
    in.setVersion(QDataStream::Qt_5_14);

    in >> header;
    qDebug() << "recv message header:" << header;
    if (header == "UPLOAD_FILE")
    {
        in >> message.sender >> message.receiver >> message.fileName >> message.fileUUID >> totalBytes;
        curFileName = message.fileName;
        qDebug() << curFileName << totalBytes << header;

        QDir dir("./uploadedfile");
        if (!dir.exists() && !dir.mkpath("."))  // 不存在则创建，"."代表dir中的"./uploadedFile"
        {
            qDebug() << "make file path failed...";
            return;
        }

        QString fileSavedPath = dir.filePath(message.fileUUID);   // 文件保存的完整路径

        m_file.setFileName(fileSavedPath);
        if (!m_file.open(QIODevice::WriteOnly))
        {
            qDebug() << "open file failed...";
            return;
        }

        // 接收第一次的包
        sltReadyReadUpload();

        connect(m_clientSocket, &QTcpSocket::readyRead, this, &FileTransferThread::sltReadyReadUpload);

    } else if (header == "DOWNLOAD_FILE"){
        QString fileUUID;
        in >> fileUUID;

        qDebug() << "recv client request fileUUID:" << fileUUID;
        QDataStream out(m_clientSocket);
        out.setVersion(QDataStream::Qt_5_11);

        DbManager dbManager;
        ChatMessage message = dbManager.getFileInfo(fileUUID);
        if (message.isVaild)
        {
            QString filePath = QString(message.filePath + "/" + message.fileUUID);
            m_file.setFileName(filePath);
            qDebug() << filePath;
            if (!m_file.open(QIODevice::ReadOnly))
            {
                qDebug() << "open file failed..." << filePath;
            }

            curFileName = message.fileName;
            totalBytes = m_file.size();
            bytesSent = 0;

            out << QString("START_SEND_FILE") << curFileName << totalBytes << message.fileUUID;
            qDebug() << curFileName << totalBytes << message.fileUUID;
            sendFileByBlock();

        } else {
            out << QString("DOWNLOAD_ERROR");
            m_clientSocket->flush();
            qDebug() << "download file error...";
            return;
        }
    }
}

void FileTransferThread::sltDisconnected()
{
    if (m_file.isOpen())
        m_file.close();
    m_clientSocket->deleteLater();
    qDebug() << "thread client disconnected...";
}

void FileTransferThread::sltReadyReadUpload()
{
    qint64 bytesAvailable = m_clientSocket->bytesAvailable();
    if (bytesAvailable < 0) {
        qDebug() << "bytes recv error...";
        return;
    }
    buffer = m_clientSocket->read(bytesAvailable);
    bytesReceived += bytesAvailable;
    m_file.write(buffer);

    qDebug() << "Received and saved " << bytesReceived << " bytes.";

    // 判断文件传输是否完成
    if (bytesReceived >= totalBytes)
    {
        m_file.close();
        QFuture<void> future = QtConcurrent::run([=](){
            DbManager dbManager;
            dbManager.saveFile(message);
        });

        //  QtConcurrent异步执行saveFile，使用QFuture来跟踪操作完成的状态
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
        connect(watcher, &QFutureWatcher<void>::finished, this, [=](){
            qDebug() << "file saved";
            watcher->deleteLater();
        });

        watcher->setFuture(future);
        qDebug() << "file transfer completed...";
    }
}

void FileTransferThread::sendFileByBlock()
{
    while(!m_file.atEnd())
    {
        buffer = m_file.read(1024 * 64);
        if (buffer.isEmpty()) return;

        m_clientSocket->write(buffer, buffer.size());
        //QThread::usleep(3);
        if (!m_clientSocket->waitForBytesWritten(3000))
        {
            qDebug() << "send file data timeout...";
            return;
        }
        bytesSent += buffer.size();
        qDebug() << "sent bytes:" << bytesSent;
    }
    emit signalTransferFinished();
    qDebug() << "successfully sent file: " << curFileName;
}

