#pragma once

#include <ui/widgets/text_edit/completer/completer_text_edit.h>


class CORE_LIBRARY_EXPORT BaseTextEdit : public CompleterTextEdit
{
public:
    explicit BaseTextEdit(QWidget* _parent = nullptr);
    ~BaseTextEdit() override;

    /**
     * @brief Необходимо ли поднимать регистр первой буквы в предложении
     */
    void setCapitalizeWords(bool _capitalize);
    bool capitalizeWords() const;

    /**
     * @brief Необходимо ли корректировать проблему ДВойных ЗАглавных
     */
    void setCorrectDoubleCapitals(bool _correct);
    bool correctDoubleCapitals() const;

    /**
     * @brief Настроить форматирование выделенного текста
     */
    /** @{ */
    void setTextBold(bool _bold);
    void setTextItalic(bool _italic);
    void setTextUnderline(bool _underline);
    /** @} */

protected:
    /**
     * @brief Дополнительная функция для обработки нажатий самим редактором
     * @return Обработано ли событие
     * @note В наследниках необходимо вызывать вручную при необходимости
     */
    virtual bool keyPressEventReimpl(QKeyEvent* _event);

    /**
     * @brief Скорректировать введённый текст
     *
     * - изменить регистр текста в начале предложения, если это необходимо
     * - исправить проблему ДВойных ЗАглавных
     *
     * @return Был ли изменён текст
     */
    virtual bool updateEnteredText(const QString& _eventText);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
