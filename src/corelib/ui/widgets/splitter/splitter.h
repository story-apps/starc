#pragma once

#include <corelib_global.h>

#include <QSplitter>


/**
 * @brief Виджет сплитер
 */
class CORE_LIBRARY_EXPORT Splitter : public QSplitter
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
