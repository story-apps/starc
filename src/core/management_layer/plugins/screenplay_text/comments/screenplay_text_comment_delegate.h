#pragma once

#include <QStyledItemDelegate>


namespace Ui {

/**
 * @brief Делегат для отрисовки комментариев
 */
class ScreenplayTextCommentDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ScreenplayTextCommentDelegate(QObject* _parent = nullptr);

    /**
     * @brief Установить необходимость отрисовки комментария в режиме единственного комментария
     */
    void setSingleCommentMode(bool _isSingleComment);

    /**
     * @brief Реализуем собственную отрисовку
     */
    void paint(QPainter* _painter, const QStyleOptionViewItem& _option,
               const QModelIndex& _index) const override;
    QSize sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const override;

private:
    /**
     * @brief Необходимо ли отрисовывать комментарий в компактном виде
     */
    bool m_isSingleCommentMode = false;
};

} // namespace Ui
