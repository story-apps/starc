#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Виджет со списком картинок по заданным ключевым словам
 */
class CORE_LIBRARY_EXPORT UnsplashImagesView : public Widget
{
    Q_OBJECT

public:
    explicit UnsplashImagesView(QWidget* _parent = nullptr);
    ~UnsplashImagesView() override;

    /**
     * @brief Загрузить изображения для заданной поисковой фразы
     */
    bool loadImages(const QString& _keywords);

    /**
     * @brief Загрузить следующую страницу изображений
     */
    bool loadNextImagesPage();

    /**
     * @brief Переопределяем размер
     */
    QSize sizeHint() const override;
    int heightForWidth(int _width) const override;

signals:
    /**
     * @brief Выбрано изображение для загрузки
     */
    void imageSelected(const QString& _url, const QString& _copyright);

protected:
    /**
     * @brief Реализуем собственную отрисовку
     */
    void paintEvent(QPaintEvent* _event) override;

    /**
     * @brief Даём визуальный фидбек, что изображение было кликнуто
     */
    void mousePressEvent(QMouseEvent* _event) override;

    /**
     * @brief По нажатию на картинку, шлём сигнал о том, что пользователь её выбрал
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
