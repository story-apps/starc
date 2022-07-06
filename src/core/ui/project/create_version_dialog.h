#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

class CreateVersionDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CreateVersionDialog(QWidget* _parent = nullptr);
    ~CreateVersionDialog() override;

signals:
    /**
     * @brief Пользователь нажал кнопку создания новой версии
     */
    void createPressed(const QString& _versionName, const QColor& _color, bool _readOnly);

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
