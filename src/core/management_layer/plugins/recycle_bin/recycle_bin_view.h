#pragma once

#include <interfaces/ui/i_document_view.h>
#include <ui/widgets/widget/widget.h>


namespace Ui {

/**
 * @brief Представление параметров аудиопостановки
 */
class RecycleBinView : public Widget, public IDocumentView
{
    Q_OBJECT

public:
    explicit RecycleBinView(QWidget* _parent = nullptr);
    ~RecycleBinView() override;

    /**
     * @brief Реализация интерфейса IDocumentView
     */
    /** @{ */
    QWidget* asQWidget() override;
    void setEditingMode(ManagementLayer::DocumentEditingMode _mode) override;
    /** @} */

    /**
     * @brief Задать количество удалённых документов
     */
    void setDocumentsToRemoveSize(int _size);

signals:
    /**
     * @brief Нажата кнопка очистки корзины
     */
    void emptyRecycleBinPressed();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
