#ifndef FRAMELESSWIDGET_H
#define FRAMELESSWIDGET_H
#include <QWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QApplication>

//////////////////////////////////////////////////
/// \brief The FramelessWidget class
/// 无边框可拖动缩放Widget
class FramelessWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FramelessWidget(QWidget *parent = nullptr)
        : QWidget(parent), m_isResizing(false), m_resizeEdge(0)
    {
        setWindowFlags(Qt::FramelessWindowHint);  // 无边框窗口
        setAttribute(Qt::WA_TranslucentBackground); // 设置背景透明
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        // 记录鼠标的起始位置和窗口的位置和大小
        m_mouseStartPos = event->globalPos();
        m_windowStartPos = this->pos();
        m_windowStartSize = this->size();

        // 判断鼠标是否在窗口边缘
        int edgeRange = 10;  // 边缘的有效范围
        QRect rect = this->rect();

        if (event->x() <= edgeRange) {
            m_isResizing = true;
            m_resizeEdge = Qt::LeftEdge;
        } else if (event->x() >= rect.width() - edgeRange) {
            m_isResizing = true;
            m_resizeEdge = Qt::RightEdge;
        } else if (event->y() <= edgeRange) {
            m_isResizing = true;
            m_resizeEdge = Qt::TopEdge;
        } else if (event->y() >= rect.height() - edgeRange) {
            m_isResizing = true;
            m_resizeEdge = Qt::BottomEdge;
        } else {
            // 鼠标位于窗口内部时，可以拖动窗口
            m_isDragging = true;
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        // 判断是否正在调整窗口大小
        if (m_isResizing) {
            int dx = event->globalPos().x() - m_mouseStartPos.x();
            int dy = event->globalPos().y() - m_mouseStartPos.y();

            // 根据边缘来调整窗口大小
            if (m_resizeEdge == Qt::LeftEdge) {
                int newWidth = m_windowStartSize.width() - dx;
                if (newWidth > minimumWidth()) {
                    setGeometry(m_windowStartPos.x() + dx, m_windowStartPos.y(), newWidth, m_windowStartSize.height());
                }
            } else if (m_resizeEdge == Qt::RightEdge) {
                int newWidth = m_windowStartSize.width() + dx;
                if (newWidth > minimumWidth()) {
                    setGeometry(m_windowStartPos.x(), m_windowStartPos.y(), newWidth, m_windowStartSize.height());
                }
            } else if (m_resizeEdge == Qt::TopEdge) {
                int newHeight = m_windowStartSize.height() - dy;
                if (newHeight > minimumHeight()) {
                    setGeometry(m_windowStartPos.x(), m_windowStartPos.y() + dy, m_windowStartSize.width(), newHeight);
                }
            } else if (m_resizeEdge == Qt::BottomEdge) {
                int newHeight = m_windowStartSize.height() + dy;
                if (newHeight > minimumHeight()) {
                    setGeometry(m_windowStartPos.x(), m_windowStartPos.y(), m_windowStartSize.width(), newHeight);
                }
            }
        }
        // 判断是否在拖动窗口
        else if (m_isDragging) {
            int dx = event->globalPos().x() - m_mouseStartPos.x();
            int dy = event->globalPos().y() - m_mouseStartPos.y();
            move(m_windowStartPos.x() + dx, m_windowStartPos.y() + dy);
        }

        // 更新光标样式
        int edgeRange = 10;  // 边缘的有效范围
        QRect rect = this->rect();

        if (event->x() <= edgeRange) {
            setCursor(Qt::SizeHorCursor);  // 左边缘：水平调整大小
        } else if (event->x() >= rect.width() - edgeRange) {
            setCursor(Qt::SizeHorCursor);  // 右边缘：水平调整大小
        } else if (event->y() <= edgeRange) {
            setCursor(Qt::SizeVerCursor);  // 上边缘：垂直调整大小
        } else if (event->y() >= rect.height() - edgeRange) {
            setCursor(Qt::SizeVerCursor);  // 下边缘：垂直调整大小
        } else {
            setCursor(Qt::ArrowCursor);  // 否则显示箭头光标
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        Q_UNUSED(event);
        m_isResizing = false;
        m_isDragging = false;
    }

private:
    bool m_isResizing;
    bool m_isDragging;
    int m_resizeEdge;
    QPoint m_mouseStartPos;
    QPoint m_windowStartPos;
    QSize m_windowStartSize;
};

#endif // FRAMELESSWIDGET_H
