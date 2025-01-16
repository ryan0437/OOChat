#include "filetransfer.h"

#include <QDebug>
#include <QFileInfo>
#include <QDataStream>
#include <QAbstractSocket>
#include <QDir>
#include <QThread>

FileTransfer::FileTransfer(QObject *parent) :
    QObject(parent),
    totalBytes(0),
    bytesSent(0),
    bytesReceived(0),
    buffer(0)
{
    m_serverSocket = new QTcpSocket(this);
    m_serverSocket->connectToHost("127.0.0.1", 9998);

    connect(m_serverSocket, &QTcpSocket::connected, this, &FileTransfer::sltConnected);
    connect(m_serverSocket, &QTcpSocket::disconnected, this, &FileTransfer::sltDisconnected);
}

void FileTransfer::sendFile(const ChatMessage& message)
{
    // 以队列方式将QString filepath传递到槽函数sltSendFile中
    QMetaObject::invokeMethod(this, "sltSendFile", Qt::QueuedConnection, Q_ARG(ChatMessage, message));
}

void FileTransfer::recvFile(const QString& fileUUID, int mode)
{
    QMetaObject::invokeMethod(this, "sltRecvFile", Qt::QueuedConnection, Q_ARG(QString, fileUUID), Q_ARG(int, mode));
}

qint64 FileTransfer::getTotalBytes()
{
    return totalBytes;
}

qint64 FileTransfer::getBytesSent()
{
    return bytesSent;
}

qint64 FileTransfer::getBytesReceived()
{
    return bytesReceived;
}

void FileTransfer::sltConnected()
{
    qDebug() << "file socket connected...";
}

void FileTransfer::sltDisconnected()
{
    if (m_file.isOpen())
        m_file.close();
    qDebug() << "file socket disconnected...";
}

void FileTransfer::sltReadyReadResponse()
{
    QDataStream in(m_serverSocket);
    QString header;
    in >> header;
    if (header == "START_SEND_FILE")
    {
        qDebug() << "start to recv file...";
        QString fileUUID;
        in >> curFileName >> totalBytes >> fileUUID;
        qDebug() << "啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊啊         " << totalBytes <<  fileUUID << header;

        QString filePath = transferMode == 1 ? QString("./downloadedfile") : QString("./chatpics");
        QDir dir(filePath);
        if (!dir.exists() && !dir.mkpath("."))
        {
            qDebug() << "make file path failed...";
            return;
        }

        QString fileSavedPath = transferMode == 1 ? dir.filePath(curFileName) : dir.filePath(fileUUID);
        m_file.setFileName(fileSavedPath);
        if (!m_file.open(QIODevice::WriteOnly))
        {
            qDebug() << "open file failed..." << fileSavedPath;
            return;
        }

        sltReadyReadDownload();
        disconnect(m_serverSocket, &QTcpSocket::readyRead, this, &FileTransfer::sltReadyReadResponse);
        connect(m_serverSocket, &QTcpSocket::readyRead, this, &FileTransfer::sltReadyReadDownload);
    } else if (header == "DOWNLOAD_ERROR"){
        qDebug() << "download file error...";
    }
}

void FileTransfer::sltReadyReadDownload()
{
    qint64 bytesAvailable = m_serverSocket->bytesAvailable();
    if (bytesAvailable < 0){
        qDebug() << "download file error...";
        return;
    }

    buffer = m_serverSocket->read(bytesAvailable);
    bytesReceived += buffer.size();
    m_file.write(buffer);

    qDebug() << "Received and saved " << bytesReceived << "bytes.";

    if (bytesReceived == totalBytes)
    {
        m_file.close();
        emit signalDownloadFinished();
        qDebug() << "file transfer completed...";
    }
}

void FileTransfer::sendFileByBlock()
{
    while (!m_file.atEnd())
    {
        buffer = m_file.read(1024 * 64);
        if (buffer.isEmpty()) return;

        m_serverSocket->write(buffer, buffer.size());
        //QThread::usleep(3);
        if (!m_serverSocket->waitForBytesWritten(3000))
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


void FileTransfer::sltSendFile(const ChatMessage &message)
{
    qDebug() << "sltSendFile started..." << message.filePath;
    m_file.setFileName(message.filePath);
    if (!m_file.open(QIODevice::ReadOnly))
    {
        qDebug() << "open file failed..." << message.filePath;
        return;
    }

    curFileName = m_file.fileName();
    totalBytes = m_file.size();
    bytesSent = 0;

    QDataStream out(m_serverSocket);
    out.setVersion(QDataStream::Qt_5_14);

    out << QString("UPLOAD_FILE") << message.sender << message.receiver << message.fileName << message.fileUUID << totalBytes;
    QThread::usleep(300);
    if (!m_serverSocket->waitForBytesWritten(3000))
    {
        qDebug() << "send file data timeout...";
        return;
    }
    sendFileByBlock();
}

void FileTransfer::sltRecvFile(const QString &fileUUID, int mode)
{
    QDataStream out(m_serverSocket);
    out.setVersion(QDataStream::Qt_5_14);
    out << QString("DOWNLOAD_FILE") << fileUUID;
    transferMode = mode;

    connect(m_serverSocket, &QTcpSocket::readyRead, this, &FileTransfer::sltReadyReadResponse);
}


/////////////////////////////////////////////////////
/// \brief ResourceTransfer::ResourceTransfer 上传/下载 资源文件
///
ResourceTransfer::ResourceTransfer(QObject *parent) : QObject(parent)
{
}

void ResourceTransfer::uploadResource(const ChatMessage &message)
{
    m_workerThread = new QThread(this);
    m_fileTransfer = new FileTransfer();

    m_fileTransfer->moveToThread(m_workerThread);

    connect(m_fileTransfer, &FileTransfer::signalTransferFinished, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_fileTransfer, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);
    connect(m_fileTransfer, &FileTransfer::signalTransferFinished, this, [=](){
        emit signalUploadFinished();
    });
    m_fileTransfer->sendFile(message);
    m_workerThread->start();
}

void ResourceTransfer::downloadResource(const QString &avatarUUID)
{
    m_workerThread = new QThread(this);
    m_fileTransfer = new FileTransfer();

    m_fileTransfer->moveToThread(m_workerThread);

    connect(m_fileTransfer, &FileTransfer::signalDownloadFinished, m_workerThread, &QThread::quit);
    connect(m_workerThread, &QThread::finished, m_fileTransfer, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);
    connect(m_fileTransfer, &FileTransfer::signalDownloadFinished, this, [=](){
        QString avatarPath = "./chatpics/" + avatarUUID;
        emit signalResourceDownloaded(avatarPath);
    });

    m_fileTransfer->recvFile(avatarUUID, 0);
    m_workerThread->start();
}
