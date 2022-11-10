#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
struct DocumentImage;
}


namespace Ui {

/**
 * @brief Виджет предпросмотра списка фотографий
 */
class CORE_LIBRARY_EXPORT ImagesListPreview : public Widget
{
    Q_OBJECT

public:
    explicit ImagesListPreview(QWidget* _parent = nullptr);
    ~ImagesListPreview() override;

    /**
     * @brief Задать список изображений
     */
    void setImages(const QVector<Domain::DocumentImage>& _images);

    /**
     * @brief Открыть предпросмотр заданного изображения
     */
    void showPreview(int _imageIndex, const QRectF& _sourceRect = {});

    /**
     * @brief Задать исходную область расположения текущего отображаемого изображения
     */
    void setCurrentImageSourceRect(const QRectF& _sourceRect);

    /**
     * @brief Закрыть предпросмотр
     */
    void hidePreview();

signals:
    /**
     * @brief Пользтеперь перешёл к просмотру другого изображения
     */
    void currentItemIndexChanged(int _itemIndex);

protected:
    bool event(QEvent* _event) override;

    /**
     * @brief Отслеживаем изменение размера родителя
     */
    bool eventFilter(QObject* _watched, QEvent* _event) override;

    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Реализуем навигацию при помощи клавиатуры
     */
    void keyPressEvent(QKeyEvent* _event) override;

    /**
     * @brief Переопределяем, чтобы реализовать интерактив
     */
    void mouseMoveEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
