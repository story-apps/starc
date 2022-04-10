#pragma once

#include <interfaces/ui/i_document_view.h>
#include <ui/abstract_navigator.h>

class QAbstractItemModel;
typedef QList<QModelIndex> QModelIndexList;


namespace Ui {

/**
 * @brief Представление навигатора по аудиопостановке
 */
class AudioplayTextStructureView : public AbstractNavigator, public IDocumentView
{
    Q_OBJECT

public:
    explicit AudioplayTextStructureView(QWidget* _parent = nullptr);
    ~AudioplayTextStructureView() override;

    /**
     * @brief Реализация интерфейса IDocumentView
     */
    QWidget* asQWidget() override;

    /**
     * @brief Настроить навигатор в соответствии с параметрами заданными в настройках
     */
    void reconfigure();

    /**
     * @brief Задать заголовок навигатора
     */
    void setTitle(const QString& _title) override;

    /**
     * @brief Задать модель сцен сценария
     */
    void setModel(QAbstractItemModel* _model);

    /**
     * @brief Установить текущий выделенный элемент в модели
     */
    void setCurrentModelIndex(const QModelIndex& _sourceIndex, const QModelIndex& _mappedIndex);

    /**
     * @brief Список выделенных элементов
     */
    QModelIndexList selectedIndexes() const;

signals:
    /**
     * @brief Пользователь выбрал элемент в навигаторе с заданным индексом
     */
    void currentModelIndexChanged(const QModelIndex& _index);

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
