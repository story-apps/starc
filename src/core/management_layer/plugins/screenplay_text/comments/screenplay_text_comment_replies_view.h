#pragma once

#include <ui/widgets/widget/widget.h>

namespace Ui {

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
     * @brief Индекс текущего отображаемого комментария
     */
    QModelIndex commentIndex() const;

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
    /**
     * @brief Отлавливаем события ввода комментария
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы по ескейп выходить из экрана
     */
    void keyPressEvent(QKeyEvent* _event) override;

    /**
     * @brief Обновляем переводы вьюхи
     */
    void updateTranslations() override;

    /**
     * @brief Наводим красоту, если сменилась дизайн система
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    /**
     * @brief Обработать нажатие пользователем кнопки ответа
     */
    void postReply();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
