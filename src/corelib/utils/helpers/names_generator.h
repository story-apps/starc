#pragma once

#include <QScopedPointer>

#include <corelib_global.h>

class TextField;


/**
 * @brief Генератор имён персонажей
 */
class CORE_LIBRARY_EXPORT NamesGenerator
{
public:
    /**
     * @brief Список доступных типов имён
     */
    static const QVector<QString> types();

    /**
     * @brief Сгенерировать имя
     * @param _type - тип имён из списка доступных
     * @param _gender - пол: 0 - оба, 1 - мужской, 2 - женский
     * @return
     */
    static QString generate(const QString& _type, int _gender = 0);

    /**
     * @brief Связать с текстовым болем
     */
    static void bind(TextField* _textField);
    static void unbind(TextField* _textField);

public:
    ~NamesGenerator();

private:
    NamesGenerator();
    static const NamesGenerator& instance();

    class Implementation;
    QScopedPointer<Implementation> d;
};
