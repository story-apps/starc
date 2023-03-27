#pragma once

#include <ui/widgets/widget/widget.h>

class QAbstractItemModel;


namespace Ui {

/**
 * @brief Панель быстрого форматирования
 */
class FastFormatWidget : public Widget
{
    Q_OBJECT

public:
    explicit FastFormatWidget(QWidget* _parent = nullptr);
    ~FastFormatWidget() override;

    /**
     * @brief Задать модель списка типов абзацев
     */
    void setParagraphTypesModel(QAbstractItemModel* _model);

    /**
     * @brief Задать название типа текущего параграфа
     */
    void setCurrentParagraphType(const QModelIndex& _index);

signals:
    /**
     * @brief Пользователь хочет сменить тип текущего абзаца
     */
    void paragraphTypeChanged(const QModelIndex& _index);

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui
