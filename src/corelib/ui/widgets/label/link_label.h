#pragma once

#include "label.h"


/**
 * @brief Базовый класс для текстовой метки с ссылкой
 */
class CORE_LIBRARY_EXPORT AbstractLinkLabel : public AbstractLabel
{
public:
    explicit AbstractLinkLabel(QWidget* _parent);
    ~AbstractLinkLabel() override;

    /**
     * @brief Задать ссылку
     */
    void setLink(const QUrl& _link);

protected:
    /**
     * @brief Переопределяем для открытия ссылки при клике на виджет
     */
    void mouseReleaseEvent(QMouseEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};


/**
 * @brief Класс текстовой метки со ссылкой
 */
class CORE_LIBRARY_EXPORT Body1LinkLabel : public AbstractLinkLabel
{
    Q_OBJECT

public:
    explicit Body1LinkLabel(QWidget* _parent = nullptr);

protected:
    const QFont& textFont() const override;
};
