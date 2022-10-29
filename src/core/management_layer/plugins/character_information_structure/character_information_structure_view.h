#pragma once

#include <interfaces/ui/i_document_view.h>
#include <ui/abstract_navigator.h>

class QAbstractItemModel;
typedef QList<QModelIndex> QModelIndexList;


namespace Ui {

class CharacterInformationStructureView : public AbstractNavigator, public IDocumentView
{
    Q_OBJECT

public:
    explicit CharacterInformationStructureView(QWidget* _parent = nullptr);
    ~CharacterInformationStructureView() override;

    /**
     * @brief Реализация интерфейса IDocumentView
     */
    /** @{ */
    QWidget* asQWidget() override;
    /** @} */

    /**
     * @brief Задать заголовок навигатора
     */
    void setTitle(const QString& _title) override;

signals:
    /**
     * @brief Пользователь выбрал группу параметров
     */
    void traitsCategoryIndexChanged(const QModelIndex& _index);

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
