#pragma once

#include <ui/widgets/text_edit/spell_check/spell_check_text_edit.h>

class QAbstractItemModel;
class Completer;


/**
 * @brief Класс текстового редактора с подстановщиком для завершения текста
 */
class CompleterTextEdit : public SpellCheckTextEdit
{
    Q_OBJECT

public:
    explicit CompleterTextEdit(QWidget* _parent = nullptr);
    ~CompleterTextEdit() override;

    /**
     * @brief Установить необходимость использования подстановщика
     */
    void setUseCompleter(bool _use);

    /**
     * @brief Получить подстановщика
     */
    Completer* completer() const;

    /**
     * @brief Открыт ли подстановщик
     */
    bool isCompleterVisible() const;

    /**
     * @brief Показать автодополнение текста
     * @return Есть ли в модели для дополнения элементы с таким текстом
     */
    bool complete(QAbstractItemModel* _model, const QString& _completionPrefix);
    bool complete(QAbstractItemModel* _model, const QString& _completionPrefix, int _cursorMovement);

    /**
     * @brief Применить выбранный в подстановщике элемент
     */
    /** @{ */
    void applyCompletion();
    void applyCompletion(const QString& _completion);
    /** @} */

    /**
     * @brief Закрыть подстановщика, если открыт
     */
    void closeCompleter();

signals:
    /**
     * @brief Показан подстановщик
     */
    void popupShowed();

    /**
     * @brief Текст был дополнен
     */
    void completed();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
