#pragma once

#include "abstract_dialog.h"


/**
 * @brief Класс простого диалога для уточняющих вопросов
 */
class CORE_LIBRARY_EXPORT Dialog : public AbstractDialog
{
    Q_OBJECT

public:
    enum ButtonType {
        NormalButton,
        AcceptButton,
        RejectButton
    };

    /**
     * @brief Структура для описания кнопок в диалоге
     */
    struct ButtonInfo {
        int id;
        QString text;
        ButtonType type;
    };

public:
    explicit Dialog(QWidget* _parent = nullptr);
    ~Dialog() override;

    /**
     * @brief Показать диалог с заданными заголовком, описанием и списком кнопок
     */
    void showDialog(const QString& _title, const QString& _supportingText, const QVector<ButtonInfo>& _buttons);

signals:
    /**
     * @brief Диалог завершился с установленным выбором пользователя
     */
    void finished(const ButtonInfo& _pressedButton);

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};
