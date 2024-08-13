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
    const QFont& textFontImpl() const override;
};


/**
 * @brief Класс текстовой метки со ссылкой
 */
class CORE_LIBRARY_EXPORT Body2LinkLabel : public AbstractLinkLabel
{
    Q_OBJECT

public:
    explicit Body2LinkLabel(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};


/**
 * @brief Класс текстовой метки со ссылкой
 */
class CORE_LIBRARY_EXPORT Subtitle2LinkLabel : public AbstractLinkLabel
{
    Q_OBJECT

public:
    explicit Subtitle2LinkLabel(QWidget* _parent = nullptr);

protected:
    const QFont& textFontImpl() const override;
};
