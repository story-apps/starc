#pragma once

#include <ui/widgets/dialog/abstract_dialog.h>


namespace Domain {
enum class DocumentObjectType;
}

namespace Ui {

/**
 * @brief Диалог создания нового документа в проекте
 */
class CreateDocumentDialog : public AbstractDialog
{
    Q_OBJECT

public:
    explicit CreateDocumentDialog(QWidget* _parent = nullptr);
    ~CreateDocumentDialog() override;

    /**
     * @brief Выбрать документ заданного типа
     */
    void setDocumentType(Domain::DocumentObjectType _type);

    /**
     * @brief Установить возможность вставки элемента в родителя
     */
    void setInsertionParent(const QString& _parentName);

    /**
     * @brief Желает ли пользователь вставлять элемент в заданного родителя
     */
    bool needInsertIntoParent() const;

signals:
    /**
     * @brief Пользователь хочет создать документ заданного типа с заданным именем
     */
    void createPressed(Domain::DocumentObjectType _type, const QString& _name);

protected:
    /**
     * @brief Определим виджет, который необходимо сфокусировать после отображения диалога
     */
    QWidget* focusedWidgetAfterShow() const override;

    /**
     * @brief Опеределим последний фокусируемый виджет в диалоге
     */
    QWidget* lastFocusableWidget() const override;

    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем UI при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
