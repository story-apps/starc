#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


/**
 * @brief Диалог обрезки изображения
 */
class ImageCroppingDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit ImageCroppingDialog(QWidget* _parent = nullptr);
    ~ImageCroppingDialog() override;

    /**
     * @brief Задать изображение
     */
    void setImage(const QPixmap& _image);

    /**
     * @brief Установить пропорцию области кадрирования
     */
    void setImageProportion(const QSizeF& _proportion);

    /**
     * @brief Установить необходимость фиксировать пропорцию кадрирования
     */
    void setImageProportionFixed(bool _fixed);

    /**
     * @brief Задать подсказку по обрезке изображения
     */
    void setImageCroppingText(const QString& _text);

signals:
    /**
     * @brief Изображение выбрана
     */
    void imageSelected(const QPixmap& _image);

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
