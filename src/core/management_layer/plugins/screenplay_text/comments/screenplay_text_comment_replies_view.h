#pragma once

#include <ui/widgets/widget/widget.h>

namespace Ui
{

/**
 * @brief Виджет обсуждения комментария
 */
class ScreenplayTextCommentRepliesView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTextCommentRepliesView(QWidget* _parent);
    ~ScreenplayTextCommentRepliesView() override;

    /**
     * @brief Установить индекс комментария для отображения
     */
    void setCommentIndex(const QModelIndex& _index);

signals:
    /**
     * @brief Пользователь хочет закрыть экран обсуждения
     */
    void closePressed();

    /**
     * @brief Пользователь хочет добавить ответ
     */
    void addReplyPressed(const QString& _reply);

protected:
    void updateTranslations() override;
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
