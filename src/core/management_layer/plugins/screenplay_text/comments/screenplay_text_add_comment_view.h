#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Виджет добавления комментария
 */
class ScreenplayTextAddCommentView : public Widget
{
    Q_OBJECT

public:
    explicit ScreenplayTextAddCommentView(QWidget* _parent = nullptr);
    ~ScreenplayTextAddCommentView() override;

    /**
     * @brief Текст комментария
     */
    QString comment() const;
    void setComment(const QString& _comment);

signals:
    /**
     * @brief Пользователь нажал кнопку сохранить
     */
    void savePressed();

    /**
     * @brief Пользователь нажал кнопку отмена
     */
    void cancelPressed();

protected:
    /**
     * @brief Отлавливаем события ввода комментария
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Обновляем переводы вьюхи
     */
    void updateTranslations() override;

    /**
     * @brief Наводим красоту, если сменилась дизайн система
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
