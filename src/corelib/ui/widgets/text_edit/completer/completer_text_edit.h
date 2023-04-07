#pragma once

#include <ui/widgets/text_edit/spell_check/spell_check_text_edit.h>

class QAbstractItemDelegate;
class QAbstractItemModel;
class Completer;


/**
 * @brief Класс текстового редактора с подстановщиком для завершения текста
 */
class CORE_LIBRARY_EXPORT CompleterTextEdit : public SpellCheckTextEdit
{
    Q_OBJECT

public:
    explicit CompleterTextEdit(QWidget* _parent = nullptr);
    ~CompleterTextEdit() override;

    /**
     * @brief Установить необходимость использования подстановщика
     */
    bool isCompleterActive() const;
    void setCompleterActive(bool _use);

    /**
     * @brief Автоматическое дополнение
     */
    bool isAutoCompleteEnabled() const;
    void setAutoCompleteEnabled(bool _enabled);

    /**
     * @brief Задать делегат для отрисовки элементов выпадающего списка
     */
    void setCompleterItemDelegate(QAbstractItemDelegate* _delegate);

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
    bool complete(QAbstractItemModel* _model, const QString& _completionPrefix,
                  int _cursorMovement);
    bool complete(QAbstractItemModel* _model, const QString& _completionPrefix, int _cursorMovement,
                  Qt::MatchFlags _filterMode);

    /**
     * @brief Применить выбранный в подстановщике элемент
     */
    /** @{ */
    void applyCompletion();
    void applyCompletion(const QModelIndex& _completionIndex);
    /** @} */

    /**
     * @brief Закрыть подстановщика, если открыт
     */
    void closeCompleter();

signals:
    /**
     * @brief Показан подстановщик
     */
    void popupShown();

    /**
     * @brief Текст был дополнен
     */
    void completed(const QModelIndex& _completionIndex);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
