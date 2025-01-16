#ifndef CUSTOMBADGE_H
#define CUSTOMBADGE_H

#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QFont>
#include <QString>
#include <QDebug>

/////////////////////////////////////////////////////////
/// \brief The WidgetBadge class
/// 显示未读信息条数Widget
///
class WidgetBadge : public QWidget {
    Q_OBJECT

public:
    explicit WidgetBadge(QWidget *parent = nullptr) : QWidget(parent), m_number(0) {this->setVisible(false);}

    void unreadIncrement(int value = 1) {   // 添加未读消息条数
        m_number += value;

        this->setVisible(true);
        updateText();
        update();
    }

    void minusRead(int readCount) {     // 减去未读消息条数
        m_number -= readCount;
        if (m_number == 0)
            this->setVisible(false);

        updateText();
        qDebug() << "curnummmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm" << m_number << m_text;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing, true);

        // 字体
        QFont font("Microsoft YaHei");
        font.setBold(true);
        font.setPointSize(7);
        painter.setFont(font);

        // 获取文字的宽度和高度
        QFontMetrics metrics(font);
        int m_textWidth = metrics.horizontalAdvance(m_text);
        int m_textHeight = metrics.height();

        // 确定背景形状和大小
        int padding = 8;
        int width, height;
        if (m_number < 10) {
            width = height = m_textWidth + padding * 2;
        } else {
            width = m_textWidth + padding * 2;
            height = m_textHeight + padding;
        }
        setFixedSize(width, height);

        // 背景
        QRectF backgroundRect(0, 0, width, height);
        painter.setBrush(Qt::red);
        painter.setPen(Qt::NoPen);
        if (m_number < 10)
            painter.drawEllipse(backgroundRect);    // 个位数时为圆形
        else
            painter.drawRoundedRect(backgroundRect, height / 2, height / 2);    // 非个位数为胶囊样式

        // 绘制文字
        painter.setPen(Qt::white);
        painter.drawText(backgroundRect, Qt::AlignCenter, m_text);
    }

private:
    int m_number;
    QString m_text;

    // 更新文本
    void updateText() {
        if (m_number > 99)
            m_text = "99+";
        else
            m_text = QString::number(m_number);
    }
};

#endif // CUSTOMBADGE_H
