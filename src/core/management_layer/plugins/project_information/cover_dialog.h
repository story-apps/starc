#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Ui {

/**
 * @brief Диалог обрезки изображения обложки
 */
class CoverDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CoverDialog(QWidget* _parent = nullptr);
    ~CoverDialog() override;

    /**
     * @brief Задать обложку
     */
    void setCover(const QPixmap& _cover);

signals:
    /**
     * @brief Обложка выбрана
     */
    void coverSelected(const QPixmap& _cover);

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
