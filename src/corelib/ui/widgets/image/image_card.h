#pragma once

#include <ui/widgets/card/card.h>

/**
 * @brief Виджет карточки изображения
 */
class CORE_LIBRARY_EXPORT ImageCard : public Card
{
    Q_OBJECT

public:
    explicit ImageCard(QWidget* _parent = nullptr);
    ~ImageCard() override;

    /**
     * @brief Задать иконку декорирования, которая отображается, если не задано изображение
     */
    void setDecorationIcon(const QString& _icon);

    /**
     * @brief Задать текст отображаемый под декорацией
     */
    void setDecorationText(const QString& _text);

    /**
     * @brief Задать подсказку для диалога обрезки изображения
     */
    void setImageCroppingText(const QString& _text);

    /**
     * @brief Задать изображение
     */
    void setImage(const QPixmap& _image);

signals:
    /**
     * @brief Изображение сменилось
     */
    void imageChanged(const QPixmap& _image);

protected:
    /**
     * @brief Реализуем отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Корректируем размер отображаемого изображения при изменении размера виджета
     */
    void resizeEvent(QResizeEvent* _event) override;

    /**
     * @brief Реализуем эффекст отображения оверлея при наведении мыши
     */
    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    /**
     * @brief Испускаем сигнал о клике пользователя
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Определяем переводы
     */
    void updateTranslations() override;

    /**
     * @brief Кореектируем размер постера
     */
    void designSystemChangeEvent(DesignSystemChangeEvent *_event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

