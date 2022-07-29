#pragma once

#include <ui/widgets/widget/widget.h>

namespace Ui {

/**
 * @brief Виджет обсуждения комментария
 */
class CommentRepliesView : public Widget
{
    Q_OBJECT

public:
    explicit CommentRepliesView(QWidget* _parent);
    ~CommentRepliesView() override;

    /**
     * @brief Задать возможность редактирования
     */
    void setReadOnly(bool _readOnly);

    /**
     * @brief Задать параметры текущего пользователя
     */
    void setCurrentUser(const QString& _name, const QString& _email);

    /**
     * @brief Задать изменение сообщения
     */
    void changeMessage(int _replyIndex, const QString& _text);

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

    /**
     * @brief Пользователь хочет обновить текст ответа
     */
    void editReplyPressed(int _replyIndex, const QString& _reply);

    /**
     * @brief Пользователь хочет отобразить контекстное меню для ответа с заданным индексом
     */
    void replyContextMenuRequested(int _replyIndex);

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
