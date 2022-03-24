#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Виджет добавления закладки
 */
class AddBookmarkView : public Widget
{
    Q_OBJECT

public:
    explicit AddBookmarkView(QWidget* _parent = nullptr);
    ~AddBookmarkView() override;

    /**
     * @brief Названаие закладки
     */
    QString name() const;
    void setName(const QString& _name);

    /**
     * @brief Цвет закладки
     */
    QColor color() const;
    void setColor(const QColor& _color);

    /**
     * @brief Текст закладки
     */
    QString text() const;
    void setText(const QString& _text);

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
     * @brief Отлавливаем события нажатия энтера и эскейпа для ускорения ввода
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
