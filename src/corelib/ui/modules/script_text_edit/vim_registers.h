#pragma once

#include <QChar>
#include <QHash>
#include <QString>


namespace Ui {
namespace Vim {

/**
 * @brief Содержимое регистра
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
     * @brief Регистр по умолчанию
     */
    static QChar unnamedRegister();

    /**
     * @brief Может ли символ быть именем регистра
     */
    static bool isValidName(QChar _name);

    /**
     * @brief Сохранить текст в регистр
     * @note Заглавная буква в имени означает, что текст нужно дописать к уже имеющемуся
     */
    void setYankedContent(QChar _name, const RegisterContent& _content);
    void setDeletedContent(QChar _name, const RegisterContent& _content);

    /**
     * @brief Получить содержимое регистра
     */
    RegisterContent content(QChar _name) const;

private:
    /**
     * @brief Сохранить содержимое в конкретный регистр
     */
    void setContent(QChar _name, const RegisterContent& _content);

    /**
     * @brief Синхронизация регистра по умолчанию с системным буфером обмена
     */
    void syncToClipboard(const RegisterContent& _content);
    RegisterContent unnamedContent() const;

    QHash<QChar, RegisterContent> m_registers;

    /**
     * @brief Текст, который мы последним положили в системный буфер обмена
     * @note Нужен, чтобы понять, что буфер обмена сменился в другом приложении
     */
    QString m_clipboardText;
};

} // namespace Vim
} // namespace Ui
