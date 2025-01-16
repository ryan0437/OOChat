#ifndef TREEITEM_H
#define TREEITEM_H

#include <QObject>
#include <QWidget>

class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QFont;
class roundedLabel;

///////////////////////////////////////////
/// \brief The ItemSort class
/// 构造好友/群组列表树中的Item
class ItemSort : public QWidget
{
    Q_OBJECT
public:
    explicit ItemSort(QWidget *parent = nullptr);

    void setSortName(const QString& sortName);  // 设置类名
    void addSortNum();                          // 增加类中的数量

private:
    QHBoxLayout *hLayoutSort;
    QLabel *labelSortName;
    QLabel *labelSortNum;
    QFont font;

};

////////////////////////////////////////
/// \brief The ItemUser class
/// 好友/群组列表中Item
class ItemUser : public QWidget
{
    Q_OBJECT
public:
    explicit ItemUser(QWidget *parent = nullptr);

    void setAvatar(const QString& avatarPath);
    void setName(const QString& name);

private:
    QHBoxLayout *hLayoutMain;
    QHBoxLayout *hLayoutAvatar;
    QVBoxLayout *vLayoutInfo;
    QWidget *widgetAvatar;
    roundedLabel *labelAvatar;
    QWidget *widgetInfo;
    QLabel *labelName;
    QLabel *labelStatus;
    QFont font;
};

#endif // TREEITEM_H
