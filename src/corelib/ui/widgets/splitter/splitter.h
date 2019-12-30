#pragma once

#include <QSplitter>


/**
 * @brief Виджет сплитер
 */
class Splitter : public QSplitter
{
    Q_OBJECT

public:
    explicit Splitter(QWidget* _parent = nullptr);
    ~Splitter() override;

    /**
     * @brief Задать цвет фона разделителя
     */
    void setHandleColor(const QColor& _color);

protected:
    /**
     * @brief Переопределяем метод создания разделителя, чтобы создать собственный
     */
    QSplitterHandle* createHandle() override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
