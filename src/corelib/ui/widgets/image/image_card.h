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
     * @brief Задать всмомогательный текст отображаемый когда нет изображения, при наведении на
     *        установленное изображение и для вопроса об удалении изображения
     */
    void setSupportingText(const QString& _emptyImageText, const QString& _imageText,
                           const QString& _clearImageQuestion);

    /**
     * @brief Задать подсказку для диалога обрезки изображения
     */
    void setImageCroppingText(const QString& _text);

    /**
     * @brief Изображение
     */
    QPixmap image() const;
    void setImage(const QPixmap& _image);
    void cropImage(const QPixmap& _image);

    /**
     * @brief Возможность редактирования изображения
     */
    bool isReadOnly() const;
    void setReadOnly(bool _readOnly);

signals:
    /**
     * @brief Изображение сменилось
     */
    void imageChanged(const QPixmap& _image);

protected:
    /**
     * @brief Получить список действий контекстного меню
     */
    virtual QVector<QAction*> contextMenuActions() const;

    /**
     * @brief Произвести обработку в наследниках на событие смены возможности редактирования
     */
    virtual void processReadOnlyChange();

    /**
     * @brief Переопределяем для реализации тултипа
     */
    bool event(QEvent* _event) override;

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
#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
    void enterEvent(QEnterEvent* _event) override;
#else
    void enterEvent(QEvent* _event) override;
#endif
    void leaveEvent(QEvent* _event) override;

    /**
     * @brief Испускаем сигнал о клике пользователя
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief Переопределяются для возможности затаскивания изображений
     */
    /** @{ */
    void dragEnterEvent(QDragEnterEvent* _event) override;
    void dragMoveEvent(QDragMoveEvent* _event) override;
    void dragLeaveEvent(QDragLeaveEvent* _event) override;
    void dropEvent(QDropEvent* _event) override;
    /** @} */

    /**
     * @brief При изменении цвета текста перенастраиваем анимацию иконки
     */
    void processTextColorChange() override;

    /**
     * @brief Определяем переводы
     */
    void updateTranslations() override;

    /**
     * @brief Кореектируем размер постера
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
