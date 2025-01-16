#ifndef ROUNDEDLABEL_H
#define ROUNDEDLABEL_H

#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QBitmap>
#include <QPainterPath>

/////////////////////////////////////////
/// \brief The roundedLabel class
/// 将label切割成圆形，用于显示头像
class roundedLabel : public QLabel
{
    Q_OBJECT

public:
    explicit roundedLabel(QWidget *parent = nullptr) : QLabel(parent) {}


private:

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (!pixmap()) return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        QPainterPath path;
        int radius = this->size().width() / 2;
        path.addRoundedRect(rect(), radius, radius);

        painter.setClipPath(path);

        painter.drawPixmap(rect(), *pixmap());
    }
};

#endif // ROUNDEDLABEL_H
