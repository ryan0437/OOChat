#ifndef RICHTEXTEDIT_H
#define RICHTEXTEDIT_H

#include <QTextEdit>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QTextCursor>
#include <QMimeData>
#include <QImage>
#include <QImageReader>
#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QUuid>
#include <QKeyEvent>

///////////////////////////////////////////////////
/// \brief The RichTextEdit class
/// 富文本textEdit
/// 支持显示拖动放入的图片，并且缩放成合适的大小
class RichTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    RichTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {

    }

protected:
    void dragEnterEvent(QDragEnterEvent *event) override {
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();  // 允许拖放图片
        }
    }

    void dropEvent(QDropEvent *event) override {
        const QMimeData *mimeData = event->mimeData();
        if (mimeData->hasUrls()) {
            foreach (const QUrl &url, mimeData->urls()) {
                QString filePath = url.toLocalFile();
                if (QFileInfo(filePath).exists()) {
                    QImageReader reader(filePath);
                    if (!reader.format().isEmpty()) {
                        insertImage(filePath);
                    }
                }
            }
        }
    }

private:
    void insertImage(const QString &imagePath) {
        QTextCursor cursor = textCursor();

        // 限制图片大小 超过限制就缩放
        QImage image(imagePath);
        QSize maxSize(250,250);
        QSize imageSize = image.size();

        if (imageSize.width() > maxSize.width() || imageSize.height() > maxSize.height()) {
            image = image.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        QString imgUUID = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString saveImgPath = "./chatpics/";
        QDir dir(saveImgPath);
        if (!dir.exists() && !dir.mkdir("."))
        {
            qDebug() << "make file path failed...";
            return;
        }

        image.save(saveImgPath + imgUUID + ".jpg");
        cursor.insertImage(saveImgPath + imgUUID + ".jpg");
    }
};

#endif // RICHTEXTEDIT_H
