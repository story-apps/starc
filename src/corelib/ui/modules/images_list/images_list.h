#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
struct DocumentImage;
}


namespace Ui {

/**
 * @brief Виджет ленты фотографий
 */
class CORE_LIBRARY_EXPORT ImagesList : public Widget
{
    Q_OBJECT

public:
    explicit ImagesList(QWidget* _parent = nullptr);
    ~ImagesList() override;

    /**
     * @brief Задать список изображений
     */
    void setImages(const QVector<Domain::DocumentImage>& _images);

    /**
     * @brief Задать возможность редактирования изображений
     */
    void setReadOnly(bool _readOnly);

    /**
     * @brief Переопределяем для корректного подсчёта размера в компоновщиках
     */
    QSize sizeHint() const override;
    int heightForWidth(int _width) const override;

signals:
    /**
     * @brief Пользователь добавил изображения
     */
    void imagesAdded(const QVector<QPixmap>& _images);

    /**
     * @brief Пользователь удалил заданное изображение
     */
    void imageRemoved(const QUuid& _imageUuid);

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Реализуем интерактив при помощи мыши
     */
    void leaveEvent(QEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseMoveEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;

    /**
     * @brief Обновляем отображаемые изображения при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
} // namespace Ui
