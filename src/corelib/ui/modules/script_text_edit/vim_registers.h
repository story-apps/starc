#pragma once

#include <QChar>
#include <QHash>
#include <QString>


namespace Ui {
namespace Vim {

/**
 * @brief Содержимое регистра
 * @note Построчный фрагмент всегда заканчивается переводом строки
 */
struct RegisterContent {
    QString text;
    bool linewise = false;

    bool isEmpty() const;
};

/**
 * @brief Регистры, в которые копируется текст
 */
class Registers
{
public:
    /**
     * @brief Может ли символ быть именем регистра
     */
    static bool isValidName(QChar _name);

    /**
     * @brief Сохранить текст в регистр
     * @param _name - имя регистра, пустой символ означает регистр по умолчанию
     * @param _toNumbered - сохранить ли удалённый текст в нумерованный регистр, даже если это
     *        удаление в пределах одной строки
     * @note Заглавная буква в имени означает, что текст нужно дописать к уже имеющемуся
     */
    void setYankedContent(QChar _name, const RegisterContent& _content);
    void setDeletedContent(QChar _name, const RegisterContent& _content, bool _toNumbered);

    /**
     * @brief Получить содержимое регистра
     */
    RegisterContent content(QChar _name) const;

private:
    /**
     * @brief Сохранить содержимое в конкретный регистр
     * @return Итоговое содержимое регистра
     */
    RegisterContent setContent(QChar _name, const RegisterContent& _content);

    /**
     * @brief Синхронизация регистра по умолчанию с системным буфером обмена
     */
    void syncToClipboard(const RegisterContent& _content);
    RegisterContent clipboardContent() const;

    QHash<QChar, RegisterContent> m_registers;

    /**
     * @brief Содержимое, которое мы последним положили в системный буфер обмена
     * @note Нужно, чтобы понять, что буфер обмена сменился в другом приложении
     */
    RegisterContent m_clipboardContent;
};

} // namespace Vim
} // namespace Ui
