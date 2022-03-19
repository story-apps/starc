#pragma once

#include <interfaces/ui/i_document_view.h>
#include <ui/abstract_navigator.h>

class QAbstractItemModel;
typedef QList<QModelIndex> QModelIndexList;


namespace Ui {

class ScreenplayTextStructureView : public AbstractNavigator, public IDocumentView
{
    Q_OBJECT

public:
    explicit ScreenplayTextStructureView(QWidget* _parent = nullptr);
    ~ScreenplayTextStructureView() override;

    /**
     * @brief Реализация интерфейса IDocumentView
     */
    QWidget* asQWidget() override;
    QVector<QAction*> options() const override;

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
    void setCurrentModelIndex(const QModelIndex& _index);

    /**
     * @brief Список выделенных элементов
     */
    QModelIndexList selectedIndexes() const;

signals:
    /**
     * @brief Пользователь выбрал элемент в навигаторе с заданным индексом
     */
    void currentModelIndexChanged(const QModelIndex& _index);

    /**
     * @brief Пользователь кликнул на кнопке вставки текста бита
     */
    void pasteBeatNamePressed(const QString& _name);

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
