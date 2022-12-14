#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

struct CoverTextParameters {
    QString text;
    QFont font;
    QColor color;
    Qt::Alignment align;
};

class CORE_LIBRARY_EXPORT CoverGeneratorSidebar : public Widget
{
    Q_OBJECT

public:
    explicit CoverGeneratorSidebar(QWidget* _parent = nullptr);
    ~CoverGeneratorSidebar() override;

    /**
     * @brief Очистить параметры
     */
    void clear();

    /**
     * @brief Параметры постера
     */
    /** @{ **/
    QColor textBackgroundColor() const;
    CoverTextParameters top1Text() const;
    CoverTextParameters top2Text() const;
    CoverTextParameters beforeNameText() const;
    CoverTextParameters nameText() const;
    CoverTextParameters afterNameText() const;
    CoverTextParameters creditsText() const;
    CoverTextParameters releaseDateText() const;
    CoverTextParameters websiteText() const;
    /** @} **/

signals:
    /**
     * @brief Изменился цвет перекрытия между текстом и изображением
     */
    void textBackgroundColorChanged();

    /**
     * @brief Изменились параметры постера
     */
    void textParametersChanged();

    /**
     * @brief Выбрано изображение для загрузки
     */
    void unsplashImageSelected(const QString& _url, const QString& _copyright);

    /**
     * @brief Нажата кнопка вставить изображение из буфера обмена
     */
    void pasteImageFromClipboardPressed();

    /**
     * @brief Нажата кнопка выбора локального файла для вставки
     */
    void chooseImgeFromFilePressed();

protected:
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
