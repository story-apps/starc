#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

/**
 * @brief Диалог создания/изменения закладки
 */
class CORE_LIBRARY_EXPORT BookmarkDialog : public AbstractDialog
{
    Q_OBJECT

public:
    /**
     * @brief Способ отображения диалога
     */
    enum DialogType {
        CreateNew,
        Edit,
    };

public:
    explicit BookmarkDialog(QWidget* _parent = nullptr);
    ~BookmarkDialog() override;

    /**
     * @brief Задать тип диалога, по умолчанию тип равен CreateNew
     */
    void setDialogType(DialogType _type);

    /**
     * @brief Текст закладки
     */
    QString bookmarkText() const;
    void setBookmarkText(const QString& _text);

    /**
     * @brief Цвет закладки
     */
    QColor bookmarkColor() const;
    void setBookmarkColor(const QColor& _color);

signals:
    /**
     * @brief Пользователь нажал кнопку сохранения закладки
     */
    void savePressed();

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
